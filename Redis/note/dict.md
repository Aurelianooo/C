# dict 模块源码阅读笔记

> Redis 3.0 的字典（哈希表）。key-value 数据库的底层核心:幂次方大小的哈希表 + 链表解决冲突 + 渐进式 rehash + 反向游标扫描。

---

## 一、为什么 `dictEntry` 的值是 union？

```c
typedef struct dictEntry {
    void *key;
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
    } v;
    struct dictEntry *next;
} dictEntry;
```

三个成员 `val`/`u64`/`s64` 共用同一块 8 字节空间,`dictEntry` 整体大小不变。

**核心动机**：大多数 value 是指针,但有些 value 就是小整数（计数器、编码后的 int）。让整数直接塞进原本留给指针的 8 字节,省一次堆分配、省内存、避免碎片。

配套宏（`dict.h`）：
- 指针：`dictSetVal` / `dictGetVal`
- 整数：`dictSetSignedIntegerVal` / `dictSetUnsignedIntegerVal` / `dictGetSignedIntegerVal` / `dictGetUnsignedIntegerVal`

类型判定交给外层 `redisObject` 的 `encoding`——union 只负责省空间,不记录类型。

---

## 二、为什么 `dictSetUnsignedIntegerVal` 要 `do while(0)`，而 `dictFreeKey` 不用？

### 本质：两者展开后的"语法形态"不同

- `dictSetUnsignedIntegerVal` 展开是**一条赋值表达式** → 分号会切断 if-else,必须包 do-while。
- `dictFreeKey` 展开是**一个 if 语句** → if 自带整体语句边界,天然安全。

### 悬挂 else 的例子

```c
#define SET(e, v) e->v.u64 = v;   // 不包 do-while

if (条件) SET(e, 1);
else foo();
// 展开成: if (条件) e->v.u64 = 1;;  ← 多出的分号让 if 提前结束,else 悬空,编译错
```

`do { ... } while(0)` 把语句打包成"安全的整体语句",解决悬挂 else。

判断标准：宏展开后是否是一个"安全的单个语句"。赋值/多语句宏需要 do-while；本身是 if 的宏一般不用。

---

## 三、为什么这些宏不写成 static inline 函数？

### C 没有函数重载（关键）

`dictGetVal` / `dictGetSignedIntegerVal` / `dictGetUnsignedIntegerVal` 三个宏**不同名**,函数完全能替代。用宏的真正原因是：

1. **访问 union 不同成员**：返回类型随字段走（`void*` / `int64_t` / `uint64_t`），函数必须有固定签名、要区分类型只能取三个不同名字。
2. **热路径零开销**：宏是文本替换，保证零调用栈帧；`static inline` 只是"建议内联",`-O0` 下不保证。
3. **历史习惯 + 与必须用宏的 setter 对称**。

真正的"必须宏"场景是**类型无关泛型**：如 `dictSetVal` 接受任意 `void*`,还条件调用 dup。

**最佳实践**：能函数则函数（类型检查、单次求值、可调试），如 `sdslen` 就是 static inline；只有需要"类型无关/访问 union/保证零开销"才用宏。

---

## 四、`dictGenHashFunction` 是干嘛的？

MurmurHash2 实现,对任意字节串（`const void*, int len`）算 32 位哈希,二进制安全。

**不是唯一入口**：字典通过 `dictHashKey(d,key)` → `type->hashFunction(key)` 调用,`dictGenHashFunction` 只是**字符串 key 的默认实现**（被 `_dictStringCopyHTHashFunction` 引用）。

其他哈希函数：`dictGenCaseHashFunction`（大小写不敏感）、`dictIntHashFunction`（整数）、`dictIdentityHashFunction`（恒等）。

关键点：哈希质量决定分布均匀性 → 决定冲突链表长度 → 决定查找性能。配套 `dict_hash_function_seed`（默认 5381）混入种子,防 hash collision 攻击。

---

## 五、为什么要两个哈希表 `ht[2]`？`dictRehash` 在干嘛？

### 动机：渐进式 rehash,不让扩容阻塞服务

普通哈希表扩容是一次性 `malloc 新表 + 整体搬空`,O(N) 遍历,几百万 key 会卡住几十~几百毫秒。Redis 单线程扛不住。

### 解法：两个表 + 分步搬

- `ht[0]` 旧表（逐步搬空），`ht[1]` 新表（逐步填满）；
- `rehashidx` 记录"旧表搬到第几个位置"。

### `dictRehash(d,n)` 逻辑（`dict.c:354`）

1. 前置：没在 rehash 直接返回 0；
2. 完成判定：`ht[0].used == 0` → 释放旧表、`ht[0]=ht[1]`、`rehashidx=-1`,返回 0（完成）；
3. 找非空桶：跳过 `table[rehashidx]==NULL` 的位置；
4. 搬桶：把整条链表按**新表 mask** 重新算索引，头插进 `ht[1]`,`rehashidx++`；
5. 返回 1（还没搬完）。

### 如何被分散到日常操作

`_dictRehashStep`（`dict.c:474`）：

```c
static void _dictRehashStep(dict *d) {
    if (d->iterators == 0) dictRehash(d,1);  // 每次操作搬 1 步
}
```

`dictAddRaw`/`dictFind`/`dictGenericDelete` 每次操作开头都调它,等价于"蚂蚁搬家"。另有 `dictRehashMilliseconds`（按时间片推进,供定时任务用）。

**一句话**：两个表为渐进式 rehash 服务,把 O(N) 卡顿拆成"每次操作搬一个桶",让 Redis 扩容无感知。

---

## 六、`dictEntry **table` 是二维数组吗？

**不是**。它是"指针数组"——`table` 指向一个长度 `size` 的 `dictEntry*` 数组,每个元素指向一个**冲突链表（桶）**。

```
table[0] → [entry]→[entry]→NULL   ← 桶0
table[1] → NULL                    ← 空桶
table[2] → [entry]→NULL
```

第一维 = 数组下标（`hash & sizemask` 决定），第二维 = 桶内冲突链表（`next` 串起来）。

不是"真二维数组"（连续内存、固定列宽），因为每个桶冲突数量动态不同,用"指针数组 + 各自链表"更灵活省内存。

---

## 七、key 冲突怎么理解？什么情况往后链接？

### 两种"冲突"要分清

| 情况 | 条件 | 结果 |
|------|------|------|
| **key 重复** | 桶里已有 key 与新 key **完全相等** | `_dictKeyIndex` 返回 -1 → 加不进 |
| **哈希碰撞** | 不同 key 但 `hash & sizemask` 撞同一桶 | 正常插入,串链表 |

`_dictKeyIndex` 关键是 `dictCompareKeys`（比较 key 相等）,不是"索引相同"。

### 往后链接 = 头插（`dictAddRaw`）

```c
entry->next = ht->table[index];   // 新节点 next 指向老桶头
ht->table[index] = entry;         // 桶头换成新节点
```

不同 key 撞同一桶时,重复头插,链表自然生长、越插越长,且后进的在前（顺序反转）。

### 碰撞必然性（抽屉原理）

`hash & sizemask` 只能产出 `size` 种索引,key 无穷多,必撞。哈希函数目标是"撞得均匀",不是"不撞"。

**最小碰撞例子**（整数 key,size=4,sizemask=3）：key `1`、`5`、`9` 低 2 位都是 `01`,全部落桶 1。

---

## 八、什么时候触发扩容/缩容？

### 扩容（自动,每次 insert 前检查 `_dictExpandIfNeeded`）

条件（两个同时满足）：
1. `used >= size`（负载因子 ≥ 1）；
2. 且（`dict_can_resize == 1` **或** `used/size > 5`（`dict_force_resize_ratio`））。

扩到 `used*2` 再向上取 2 的幂（`_dictNextPower`）。

### 缩容（非自动,由 serverCron 周期调 `dictResize`）

删除后负载因子过低时,缩到 `max(used, 4)` 的 2 的幂。因为"何时缩"更复杂、非高频,交给定时任务。

### 目标 & `dict_can_resize`

让负载因子维持 1 附近（时间/空间平衡）。`dict_can_resize=0` 用于 RDB/AOF fork 期间配合 copy-on-write（避免大量内存页复制）；但负载因子 > 5 时仍强制扩,防性能崩。

---

## 九、为什么设计 `dictDeleteNoFree`？

不是泄漏,是"**把 key/value 的释放责任交给调用者**"。

两者都归 `dictGenericDelete(d, key, nofree)`:
- `dictDelete`（nofree=0）：调 `dictFreeKey`/`dictFreeVal` 释放 key/value；
- `dictDeleteNoFree`（nofree=1）：**不调**析构,但 `dictEntry` 节点本身两个都释放（`zfree(he)` 在 if 外）。

用途：转移 value 所有权、引用计数共享对象、key/value 本就不归 dict 管。类比 C++ `unique_ptr::release()`。

只要调用者后续妥善处理,就没泄漏；撒手不管才会泄漏,那是调用者的责任。

---

## 十、`_dictClear` 里 `(i & 65535) == 0` 是什么？

进度回调 + 位运算技巧。

```c
if (callback && (i & 65535) == 0) callback(d->privdata);
```

- `65535 = 0xFFFF = 2^16 - 1`,`i & 65535` 取 i 低 16 位,`==0` 即 **i 是 65536 的整数倍**。
- 作用：清空大表的长循环里,每扫描 65536 个桶回调一次,通知调用方进度/让出 CPU。
- 为什么位与不取模：`x % 2^n == x & (2^n - 1)`,位与比取模快得多。

---

## 十一、安全迭代器 vs 不安全迭代器

两个创建函数实现相同,只差 `safe` 字段。

### 安全迭代器（`safe=1`）

- 遍历时递增 `d->iterators`,使 `_dictRehashStep` 因 `iterators>0` **暂停 rehash**,字典结构稳定;
- 配合 `dictNext` 里"先存 nextEntry 再返回当前",允许边遍历边增删改;
- 代价：遍历期间不能 rehash。

### 不安全迭代器（`safe=0`）

- 不递增 iterators,rehash 照常;
- 记录 `fingerprint`（把 ht[0]/ht[1] 的 table/size/used 哈希成 64 位数）;
- 释放时 `assert(指纹相符)`,若遍历中改了字典则崩,用于抓 bug;
- 只用于纯只读遍历,性能更优。

---

## 十二、`dictScan` 的反向游标算法

`SCAN`/`HSCAN`/`SSCAN` 的底层迭代。目标:**表在两次调用之间可扩缩容,仍不重不漏。**

### 1. `v |= ~m0; rev(v); v++; rev(v)` 的作用

`rev(rev(v)+1)` 把游标从**低位进位**改成**高位进位**。

size=8（3 位）时游标顺序：

```
000 → 100 → 010 → 110 → 001 → 101 → 011 → 111 → (回 000)
```

（普通 +1 是 `000→001→010→011`,这是错的）

高位进位让"表扩容时,已访问低位组合不会重复出现,新增桶由后续游标自然覆盖"。`v |= ~m0` 是把不受 mask 约束的高位先置 1,配合 rev 保证进位只作用在 mask 覆盖的位上。

### 2. 双表 do-while（rehash 中）

```c
t0 = 小表, t1 = 大表;
de = t0->table[v & m0];  while(de){...}   // 先扫小表桶

do {
    de = t1->table[v & m1];  while(de){...}  // 扫大表扩张桶
    v = (((v | m0) + 1) & ~m0) | (v & m0);   // 高位+1、低 m0 位不变
} while (v & (m0 ^ m1));                       // 高位枚举完成前继续
```

- 小表一个桶对应大表多个桶（大表低 `m0` 位相同、高位可 0 可 1），必须都扫；
- `v = (((v|m0)+1)&~m0)|(v&m0)` 保持低 m0 位不变、高位 +1,枚举大表里"低 m0 位相同"的所有扩张桶；
- `m0 ^ m1` 是"大表比小表多出的位",`while(v & (m0^m1))` 判断高位枚举是否完成。

### 3. 扩容不重不漏（size 8→16 例子）

- 阶段1（size=8, mask=111）游标走 `000→100→010→110`（桶 0,4,2,6）；
- 扩容到 size=16,阶段2 游标走 `0001→1001→0101→1101→0011→1011→0111→1111`（桶 1,9,5,13,3,11,7,15）；
- 阶段2 里**没有** `0000/1000/0100/1100`（已在阶段1 扫过）,也没漏任何一个 = 不重不漏。

---

## 十三、总结：dict 的几个核心设计

1. **union 值**：省内存,让整数 value 无需单独分配。
2. **双哈希表 + 渐进 rehash**：O(N) 扩容拆成"每次搬一桶",不阻塞。
3. **链表冲突 + 头插**：碰撞必然,靠链表解决,新数据插前面。
4. **安全/不安全迭代器**：用 iterators 计数暂停 rehash,或用 fingerprint 抓误用。
5. **反向游标 dictScan**：高位进位 + 双表扩张桶枚举,支持遍历途中变大小。
