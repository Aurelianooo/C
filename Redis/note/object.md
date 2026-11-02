# object.c 模块问答

> 对应源码:`src/object.c`、`src/redis.h`(robj 定义)
> 相关文件:`src/adlist.h`、`src/t_list.c`、`src/t_zset.c`

---

## 一、object.c 是什么?

如果说 Redis 的数据结构模块(sds / adlist / dict / ziplist / intset / skiplist)是"零件",那么 object.c 就是**把这些零件组装成"Redis 对象"的工厂和装配车间**。它定义了 `robj` 统一包装,负责:

1. 创建各种类型的对象(字符串、列表、集合、哈希、有序集合)
2. 选择编码(同一种类型用多套底层数据结构)
3. 引用计数管理(共享对象的生命周期)
4. 类型安全检查
5. 类型转换工具(从对象里取出 int / long long / double 等值)

### 核心:robj 的"双层抽象"

每个值都是一个 `robj`,有两个关键字段:

- **`type`**:逻辑类型(对外语义),如 STRING、LIST、SET、HASH、ZSET
- **`encoding`**:物理编码(内部实现),决定 `ptr` 指向哪种数据结构

同一个 `type` 对应多个 `encoding`,按数据规模/内容自动切换:

| type | 可能的 encoding | 对应底层结构 |
|------|----------------|-------------|
| STRING | RAW / INT / EMBSTR | sds / 直接存整数 / 内嵌字符串 |
| LIST | LINKEDLIST / ZIPLIST | adlist / ziplist |
| SET | HT / INTSET | dict / intset |
| HASH | HT / ZIPLIST | dict / ziplist |
| ZSET | SKIPLIST / ZIPLIST | skiplist+dict / ziplist |

之前读 intset/ziplist 时总说"一旦集合变大/出现字符串就换另一种结构",那个"换"的决策逻辑就在 object.c 和 t_*.c 里,object.c 提供"创建哪种编码"的入口函数。

---

## 二、decrRefCountVoid 存在的意义是什么?

`decrRefCountVoid` 是一个**签名适配器**:把 `decrRefCount(robj *)` 包装成 `void (*)(void *)` 的形式,使其能塞进 `list->free` 等要求泛型签名的回调槽位。

```c
void decrRefCountVoid(void *o) {
    decrRefCount(o);   // 唯一的 void* -> robj* 转换点
}
```

### 为什么需要它

`decrRefCount` 的参数是 `robj *`,而 `adlist` 的 `list->free` 字段类型是 `void (*)(void *ptr)`。在 C 里,`void (*)(robj *)` 和 `void (*)(void *)` 是**两种不同的函数指针类型**,不能直接赋值:

```c
void (*f)(void *);
f = decrRefCount;       // ❌ 类型不兼容
f = decrRefCountVoid;   // ✅ 正好
```

`createListObject`(223 行)就有 `listSetFreeMethod(l, decrRefCountVoid);`。实际全代码库 6 处调用,全部是 `listSetFreeMethod(...)`。

---

## 三、为什么不让 decrRefCount 参数直接写成 void* ,省掉包装?

因为要**保住类型安全**。

| 方案 | 效果 |
|------|------|
| `decrRefCount(robj *)` + 单独的 `decrRefCountVoid(void *)` | 绝大多数调用点享受编译期类型检查 |
| `decrRefCount(void *)` 内部强转 | 所有调用者都失去类型检查 |

`decrRefCount` 是代码库里调用最频繁的函数之一,若改成 `void *`,任何指针都能传进去,编译器不再拦截:

```c
decrRefCount(some_sds_pointer);   // 传错类型,编译不报错,运行时炸
```

保持 `robj *` 参数,传错类型会在**编译期**被抓出。包装函数把"不安全的 `void*` 转换"集中到 `decrRefCountVoid` 这一处,同时用名字注明"这是给 void* 场景用的版本"。

### 本质:C 没有函数重载/泛型

同一逻辑适配不同签名,C 只有三种办法:用 `void*`(牺牲安全)、写两个不同名函数(Redis 选这个)、用宏。Redis 宁可多写个 `decrRefCountVoid`,也要保住 `decrRefCount(robj*)` 在 99% 调用点享受类型检查。

---

## 四、tryObjectEncoding 这个函数在干嘛?

它是 Redis "能省就省"的核心。目标:拿到一个 RAW/EMBSTR 编码的字符串对象,看能否用更省内存的方式重新编码。

### 整体流程

```
tryObjectEncoding(o)
  ├─ 不是字符串 -> 返回
  ├─ 不是 RAW/EMBSTR(已是 INT)-> 返回
  ├─ 是共享对象(refcount>1)-> 返回(不能改)
  │
  ├─ 能解析成整数?
  │    ├─ 0~9999 小整数 -> 换成共享整数对象
  │    └─ 普通整数 -> 转 INT 编码(ptr 存整数)
  │
  ├─ 短字符串(<=39字节)且还是 RAW -> 换 EMBSTR
  │
  └─ 都没法编码 -> sds 有 >10% 空闲则回收
```

### 三个守卫(534~547 行)

```c
if (!sdsEncodedObject(o)) return o;   // INT 已经最省
if (o->refcount > 1) return o;        // 共享对象不能动(改了会影响所有引用者)
```

### 整数分支(554 行)

`len <= 21` 先排除:超过 21 字节不可能被 64 位整数表示。再 `string2l` 解析,成功则:

- `server.maxmemory == 0` 且 `0 <= value < REDIS_SHARED_INTEGERS` 的小整数 → 换 `shared.integers[value]` 共享对象
- 其它整数 → `o->encoding = INT; o->ptr = (void*)value`(原地改,先 sdsfree 掉旧 sds)

### EMBSTR 分支(579 行)

`len <= 39` 且还是 RAW → 换成 EMBSTR(robj + sdshdr + 内容一次 malloc)。

### 兜底(598 行)

长字符串又有 >10% 空闲 → `sdsRemoveFreeSpace` 挤掉冗余。

### 为什么 39 这个限制

39 是 `REDIS_ENCODING_EMBSTR_SIZE_LIMIT`,让最大的 EMBSTR 对象刚好塞进 jemalloc 的 64 字节 arena。

### 哲学:渐进式省,一档一档降

能变整数 → 最省;短字符串 → EMBSTR;长字符串 → 挤掉冗余。每一步命中就 `return`,是"贪心:先吃最好省的那一口"。

---

## 五、robj 为什么要设计 lru 字段?

`robj->lru` 是内存淘汰(LRU 策略)的地基。它存的不是真实时间戳,而是**一个不断自增的"时钟计数"**:

```c
o->lru = LRU_CLOCK();   // 读全局 server.lruclock(每 100ms +1)
```

用计数器而非时间戳,是因为 64 位时间戳太占内存,计数器够用(LRU 只要"相对先后")。

### 为什么每个 robj 都要有

Redis 是内存数据库,内存有限。`maxmemory` 配置超限后要淘汰 key,LRU(最近最少使用)是最经典策略:谁最久没被访问谁先删。要判断"多久没访问",就得在每个对象上记录"最后访问时刻"。

### 淘汰时怎么用

淘汰时采样一批 key,算空闲时长(`estimateObjectIdleTime`):

```c
当前 lruclock - 对象的 lru = 空闲时长
```

差值越大越该淘汰。`OBJECT idletime` 命令就是调它。

### 为什么近似 LRU 而非精确 LRU

精确 LRU 要维护全局双向链表 + 哈希表,每次访问 O(1) 移表头,代价大且是全局锁热点。Redis 用近似 LRU:采样 + 比较 lru,实现简单、开销低。

### 为什么共享会破坏 LRU

`shared.integers` 是全局唯一对象,很多 key 共享同一个实例,`lru` 字段也只有一个值。某个 key 被访问会刷新这唯一对象的 lru,导致所有引用它的 key 都"误刷"成刚访问过,淘汰判断失真。

所以 `tryObjectEncoding` 里:开了 `maxmemory`(需要精确 LRU)时,Redis 禁用共享整数对象,让每个对象有独立 lru。

---

## 六、objectCommand 是核心函数吗?

不核心,反而是 object.c 里最"外围"的函数 —— 它是 `OBJECT` 命令的入口,方便在命令行查看对象内部状态。但很适合作为理解前面概念的"观察窗"。

### 三个子命令

```
OBJECT refcount   <key>   # 看引用计数
OBJECT encoding   <key>   # 看底层编码
OBJECT idletime   <key>   # 看空闲时长
```

结构是大 if-else,靠 `strcasecmp`(忽略大小写)判断子命令分发:

- refcount → `o->refcount`
- encoding → `strEncoding(o->encoding)`(数值转可读串)
- idletime → `estimateObjectIdleTime(o)/1000`

### 值得注意:查而不动

`objectCommandLookup`(1024 行)用 `dictFind` 直接查 key,**不触发 LRU 更新**。因为 `OBJECT idletime` 只是想"看看空置多久",若这一查也刷新 lru,就永远测不出真实空闲时长了。

---

## 七、createStringObjectFromLongLong 的 ptr 复用技巧

```c
o->encoding = REDIS_ENCODING_INT;
o->ptr = (void*)((long)value);   // 把 long 硬塞进 void* 里
```

### 核心:指针被当"8 字节容器"存整数

`void *` 指针本身占 8 字节(64 位),平时存"内存地址"这个数。既然它是 8 字节的数,理论上也能存"别的 8 字节数"—— 比如一个 long 整数。

`(void*)((long)value)` 双重强转就是告诉编译器:"把 value 这 8 个 bit 当指针存进去",`ptr` 实际没指向任何内存。

### 对比存法(以整数 12345 为例)

- **存 sds**:robj(16B) + sdshdr(8B) + "12345"(6B),两次 malloc ≈ 30 字节 + 碎片
- **INT 编码**:robj 的 ptr 字段直接装 12345,一次 malloc,没有 sds、没有二次分配、没有字符串转换

INT 是最省内存的字符串编码 —— 彻底扔掉"字符串"抽象直接存数字。

### 怎么读回:处处 `(long)o->ptr`

`isObjectRepresentableAsLongLong`(513 行)、`getLongLongFromObject`(917 行)、`getDoubleFromObject`(785 行)里都是 `(long)o->ptr`,把整数从指针里抠出来。

### 怎么区分真假指针:靠 encoding

- `encoding == INT` → ptr 是整数,`(long)o->ptr` 读
- `encoding == RAW/EMBSTR` → ptr 是真指针,`sdslen(o->ptr)` 读

`encoding` 是"判读钥匙"。因此 object.c 里几乎所有函数开头先判断 `sdsEncodedObject(o)`。

### 风险

若代码忘了判断 encoding,对 INT 对象的 ptr 做 `sdslen` 解引用,会把整数当地址访问,直接段错误。所以 `freeStringObject`(322 行)对 INT 编码什么都不做 —— INT 没申请过 sds 内存,`sdsfree` 会崩。

---

## 八、redisAssertWithInfo 这个断言是干嘛的?

`redisAssertWithInfo` 是 Redis 自封装的断言宏,条件不满足时打印详细信息并终止进程。

定义(redis.h 382 行):

```c
#define redisAssertWithInfo(_c,_o,_e) ((_e)?(void)0 : (_redisAssertWithInfo(_c,_o,#_e,__FILE__,__LINE__),_exit(1)))
```

### 三个参数

| 参数 | 含义 |
|------|------|
| `_c` | 客户端信息(可能 NULL) |
| `_o` | 相关对象(可能 NULL) |
| `_e` | 要断言的条件 |

`#_e` 是 C 预处理器的"字符串化",把条件文本变成字符串,报错时能打印原始条件。`__FILE__`/`__LINE__` 是编译器提供的文件名/行号宏。

### 三个相关宏的分层

| 宏 | 用途 |
|----|------|
| `redisAssertWithInfo(_c,_o,_e)` | 断言 + 带客户端/对象上下文 |
| `redisAssert(_e)` | 断言,不带上下文 |
| `redisPanic(_e)` | 无条件报错退出 |

### 断言 vs 输入校验(重要区别)

- **断言**针对内部逻辑错误 —— 本该永远正确,错了就是 bug,应该崩
- **输入校验**针对外部非法输入 —— 用户乱传是正常的,应优雅报错

`redisPanic("Unknown list encoding type")` 是另一种:encoding 只可能有几个合法值,走到 default 说明数据损坏,主动放弃。

---

## 九、getDoubleFromObject 的浮点解析边界校验

```c
errno = 0;
value = strtod(o->ptr, &eptr);
if (isspace(((char*)o->ptr)[0]) ||      // ① 首字符空白
    eptr[0] != '\0' ||                  // ② 尾巴有残留垃圾
    (errno == ERANGE &&                 // ③ 上下溢出
        (value == HUGE_VAL || value == -HUGE_VAL || value == 0)) ||
    errno == EINVAL ||                  // ④ 完全无法解析
    isnan(value))                        // ⑤ NaN
    return REDIS_ERR;
```

五个条件逐个拆:

1. **首字符空白**:`strtod` 会自动跳过开头空白,但 Redis 不认这种输入
2. **`eptr[0] != '\0'`**(最关键):`strtod` 解析完指向第一个没解析的字符,若 `eptr[0] != '\0'` 说明后面有垃圾(如 `"3.14abc"`),拒绝
3. **溢出**:`errno == ERANGE` 且 value 变成正负无穷(上溢)或 0(下溢)
4. **EINVAL**:完全无法解析(如空串)
5. **isnan**:不存 NaN 进系统

### 为什么这么较真

这些值最终写进内存数据库、参与运算存储。放个 `"3.14abc"` 或 `Inf` 进去,做 ZADD score、INCRBYFLOAT 会产生不可预料的后果(污染排序、持久化脏数据)。

因此原则是:**在边界上(用户输入进入系统的瞬间),把可疑值拒之门外**,优雅返回 `REDIS_ERR`(由 `getDoubleFromObjectOrReply` 翻译成客户端报错),而不是 assert 崩掉。

### 孪生函数

`getLongDoubleFromObject`(828 行)是同一套路,把 double 换 long double、strtod 换 strtold,校验条件略有精简。
