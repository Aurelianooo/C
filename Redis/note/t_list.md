# t_list.c 模块问答

> 对应源码:`src/t_list.c`
> 相关文件:`src/object.c`、`src/adlist.c`、`src/ziplist.c`、`src/db.c`

---

## 一、t_list.c 是什么?

`t_list.c` 分上下两部分:

1. **上半部分(34~483 行):List API 层** —— 一组 `listTypeXxx` 函数,把列表在不同编码下的操作抽象成统一接口。
2. **下半部分(485 行起):List Commands** —— 具体命令实现 + 阻塞式 pop(BLPOP/BRPOP/BRPOPLPUSH)。

### 为什么多出"List API 层"

List 有两种底层编码 ZIPLIST / LINKEDLIST,每条命令都要写两份几乎一样的逻辑。为避免重复,先定义 `listTypeXxx` 统一接口,内部用 `if (encoding)` 分派,命令层就不关心编码细节。

List API 层函数:`listTypeTryConversion` / `listTypePush` / `listTypePop` / `listTypeLength` / `listTypeInitIterator` / `listTypeNext` / `listTypeGet` / `listTypeInsert` / `listTypeEqual` / `listTypeDelete` / `listTypeConvert`。

### 三个核心主题

1. 编码升级触发条件(ziplist → linkedlist)
2. 编码差异带来的值所有权处理(refcount / getDecodedObject)
3. 阻塞式 pop 实现

---

## 二、编码升级逻辑与 listTypeConvert

List 默认用 ziplist 省内存,但元素多或长时性能崩。两个阈值:

- `list-max-ziplist-entries`(默认 512):元素数量
- `list-max-ziplist-value`(默认 64):单个元素字节数

### 两个触发检查点

**`listTypeTryConversion`(48 行)** —— 检查单元素是否过长:

```c
if (subject->encoding != REDIS_ENCODING_ZIPLIST) return;  // 已非 ziplist 不必查
if (sdsEncodedObject(value) &&
    sdslen(value->ptr) > server.list_max_ziplist_value)   // 元素超 64 字节
        listTypeConvert(subject, REDIS_ENCODING_LINKEDLIST);
```

只对 `sdsEncodedObject` 检查长度,INT 编码整数不可能超长。

**`listTypePush`(82 行)** —— 检查元素是否过多:

```c
if (subject->encoding == REDIS_ENCODING_ZIPLIST &&
    ziplistLen(subject->ptr) >= server.list_max_ziplist_entries)
        listTypeConvert(subject, REDIS_ENCODING_LINKEDLIST);
```

### listTypeConvert(450 行)逐元素搬迁

```c
list *l = listCreate();
listSetFreeMethod(l, decrRefCountVoid);   // 链表释放时每个节点 decrRefCount

li = listTypeInitIterator(subject, 0, REDIS_TAIL);
while (listTypeNext(li, &entry))
    listAddNodeTail(l, listTypeGet(&entry));   // get 已 incrRefCount,匹配 add 的引用
listTypeReleaseIterator(li);

subject->encoding = REDIS_ENCODING_LINKEDLIST;
zfree(subject->ptr);                         // 释放旧 ziplist
subject->ptr = l;
```

### 关键:listSetFreeMethod(l, decrRefCountVoid)

链表节点存的是 robj*,释放时必须对每个 value `decrRefCount`。`decrRefCountVoid`(void* 适配壳)正是你之前学的"签名适配器"第一次发挥作用的地方。

### 单向升级

`else { redisPanic("Unsupported list conversion") }` —— 只有 ziplist → linkedlist 方向,不支持降级。与 intset"只能升级不能降级"同理:list 一旦变大很少缩回小而短,不值得为降级写反向搬迁。

---

## 三、值所有权(refcount)处理

### 根本原因:两种编码对值的存储方式不同

| | ziplist | linkedlist |
|---|---------|-----------|
| 元素是什么 | 压平的字节(无对象) | robj* 指针(有对象) |
| 有没有独立对象 | 无 | 有 |
| 引用计数 | 无(字节无 refcount) | 有 |

### 场景对照

**Push(listTypePush 76 行):**

```c
// ziplist:解码 → 拷贝字节 → 释放临时
value = getDecodedObject(value);
subject->ptr = ziplistPush(..., value->ptr, sdslen(value->ptr), pos);
decrRefCount(value);

// linkedlist:存指针 → 加引用
listAddNodeTail(subject->ptr, value);
incrRefCount(value);
```

ziplist 存"值的字节拷贝",拷进去就和原 robj 无关;链表存"对象的引用",必须以 incrRefCount 表示新引用者。

**Get(listTypeGet 296 行):**

- ziplist:字节无现成对象,`createStringObject` 现场新建
- linkedlist:拿指针 + `incrRefCount`(调用者成为新引用者)

**Delete(listTypeDelete 407 行):**

- ziplist:删字节,无 refcount 要减
- linkedlist:`listDelNode` 自动调 `list->free`(即 decrRefCountVoid),自动减引用

**Set(LSET 702 行):**

- linkedlist:旧 `decrRefCount` + 新 `incrRefCount`
- ziplist:删旧字节 + 插新字节,配合 getDecodedObject/decrRefCount

### 总结表

| 操作 | ZIPLIST | LINKEDLIST |
|------|---------|-----------|
| Push | getDecodedObject → 拷字节 → decrRefCount | 存指针 → incrRefCount |
| Get | createStringObject 新建 | 拿指针 → incrRefCount |
| Delete | 直接删字节 | listDelNode 自动 decrRefCount |
| Set | 删旧字节 + 插新字节 | 旧 decrRefCount + 新 incrRefCount |

**核心**:值是被"拷贝"了还是被"引用"了 —— ziplist 拷贝(无 refcount),linkedlist 引用(有 refcount)。

---

## 四、阻塞式 pop 机制(BLPOP/BRPOP)

语义:key 非空立即弹出;key 空则阻塞等待,有人 push 才唤醒。

### 关键数据结构

| 字段 | 作用 |
|------|------|
| `c->bpop.keys` | 客户端在等哪些 key |
| `c->bpop.timeout`/`target` | 超时 / BRPOPLPUSH 目标 key |
| `db->blocking_keys` | key → 等待它的客户端链表 |
| `db->ready_keys` | 就绪 key 去重 set |
| `server.ready_keys` | 就绪 key 待处理链表 |

### 三段式流程

**① 阻塞 blockForKeys(1139 行):**

双向记账:`c->bpop.keys` 记"客户端等哪些 key";`db->blocking_keys` 记"key 上有哪些客户端"(listAddNodeTail 尾部插入)。最后 `blockClient` 真正阻塞。

**② 标记就绪 signalListAsReady(1242 行):**

push 时把 key 标进 `server.ready_keys`,用 `db->ready_keys` 字典做 O(1) 去重(避免事务/脚本里同一 key 重复入队)。只标记不分发。

**③ 分发 handleClientsBlockedOnLists(1396 行):**

命令执行完后真正唤醒:

- **双缓冲**:`l = server.ready_keys; server.ready_keys = listCreate();` 处理旧链表,新就绪进新链表,避免迭代时被改。
- **FIFO**:`listFirst(clients)` 从头取,谁先阻塞谁先服务(blockForKeys 是尾部插入)。
- **元素不够就 break**:`listTypePop` 返回 NULL 时,剩下的客户端继续等下一波。
- **命令传播**:`serveClientBlockedOnList` 把 BLPOP/BRPOP 改写成 LPOP/RPOP 再传播(避免从库重放阻塞命令)。

### 完整流程

```
BLPOP key 5 → key 空 → blockForKeys 挂到 blocking_keys
LPUSH key "msg" → signalListAsReady 标记就绪
命令结束 → handleClientsBlockedOnLists 按 FIFO 分发 → unblockClient 唤醒
```

这是 Redis 实现"阻塞队列/消息队列"的核心。

---

## 五、lrangeCommand 的双向遍历优化

链表分支(857 行):

```c
if (start > llen/2) start -= llen;   // 关键一行
ln = listIndex(o->ptr, start);
```

### 背景:链表只能顺序访问

`listIndex` 要拿第 N 个元素只能从头/尾走 N 步,O(N)。若 start 靠近末尾,从头走很亏。

### 解法:从最近一端出发

`start > llen/2` 时把 start 转成负索引:`start -= llen`。如 start=9998、llen=10000 → start=-2,`listIndex(-2)` 从尾部倒着走只 2 步,而非 9998 步。

利用了 `listIndex` 已支持负索引的特性(负的从 tail 倒走)。定位起点从最近端出发,定位后仍正向 next 遍历(输出范围不可避免)。

### 对比 ziplist 分支

ziplist 分支没写这行,因为 `ziplistIndex` 内部已处理正负索引优化。殊途同归:都是"找元素时从更近的一端出发"。

---

## 六、rpoplpushCommand 的原子性

```
RPOPLPUSH source dest
  element = RPOP source    (从源尾部弹出)
  LPUSH dest element        (推到目标头部)
  RETURN element
```

经典用途:可靠队列的"消费 + 转移到备份队列",防止消费中崩溃丢消息。

### 原子性来源:单线程模型

Redis 单线程串行执行命令,`rpoplpushCommand` 从 pop 到 push 到返回一气呵成,中间不被其它命令打断,无需显式锁或事务标记。

### 两个细节

1. **`incrRefCount(touchedkey)`**(1065 行):保护源 key,防止后续 `rpoplpushHandlePush` 修改 argv 导致源 key 悬空,用完后 decrRefCount 释放。
2. **空列表删 key**:源空了 `dbDelete` 删掉,避免残留空 key(popGenericCommand、ltrimCommand 同款模式)。

### 原子性边界区分

- 单命令内部原子性:单线程天然保证(RPOPLPUSH 属于此层)
- 多命令原子性:需 MULTI/EXEC 或 Lua 脚本

RPOPLPUSH 把 pop+push 合二为一,借助单线程免去用户套事务。阻塞版 BRPOPLPUSH 一脉相承,只是源空时先阻塞,原子性同样靠单线程,且 `serveClientBlockedOnList` 把 RPOP+LPUSH 拆两条命令传播给从库。
