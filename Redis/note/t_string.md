# t_string.c 模块问答

> 对应源码:`src/t_string.c`
> 相关文件:`src/object.c`、`src/db.c`

---

## 一、t_string.c 是什么?

`t_` 前缀意为 "type"(类型实现),`t_string.c` 是**字符串类型对外提供的所有命令**的实现层。用户敲进来的每条 `SET`/`GET`/`INCR` 命令,都对应这里的一个 `xxxCommand` 函数。

实现的命令:

| 命令 | 处理函数 | 作用 |
|------|---------|------|
| SET | `setCommand` | 设置 key 值,支持 NX/XX/EX/PX |
| SETNX | `setnxCommand` | 仅 key 不存在时设置 |
| SETEX / PSETEX | `setexCommand` / `psetexCommand` | 设值并带过期(秒/毫秒) |
| GET | `getCommand` | 取值 |
| GETSET | `getsetCommand` | 取旧值同时设新值 |
| SETRANGE | `setrangeCommand` | 覆盖字符串某段 |
| GETRANGE | `getrangeCommand` | 取子串 |
| MGET | `mgetCommand` | 批量取值 |
| MSET / MSETNX | `msetCommand` / `msetnxCommand` | 批量设置 |
| INCR / DECR / INCRBY / DECRBY | `incrCommand` 等 | 整数自增/自减 |
| INCRBYFLOAT | `incrbyfloatCommand` | 浮点自增 |
| APPEND | `appendCommand` | 追加 |
| STRLEN | `strlenCommand` | 字符串长度 |

### 整体结构:命令 → 公共引擎 两层设计

很多命令很薄,因共同逻辑抽进了 `xxxGenericCommand`:

1. `setGenericCommand` —— SET/SETEX/PSETEX/SETNX 共用
2. `getGenericCommand` —— GET/GETSET 复用
3. `msetGenericCommand` —— MSET/MSETNX 共用
4. `incrDecrCommand` —— INCR/DECR/INCRBY/DECRBY 共用

目的:避免四条命令复制粘贴四份几乎一样的代码。

### 贯穿全文的三个主题

1. **编码优化 `tryObjectEncoding`**:写命令存值前先调,用户输入 "123" 先试转 INT 省内存。
2. **编码感知的读取**:读命令正确处理 INT 编码,如 `getrangeCommand` 里 INT 要先 `ll2string` 转字符串,RAW 直接 `sdslen`。
3. **类型检查 `checkType`**:取到值后确认是字符串,防止拿 list 当 string 用。

---

## 二、setGenericCommand 的四合一设计

一个函数同时服务四条命令,核心思想:**命令之间的差异就是参数的差异**。

```c
void setGenericCommand(redisClient *c, int flags, robj *key, robj *val,
                       robj *expire, int unit,
                       robj *ok_reply, robj *abort_reply)
```

| 命令 | flags | expire | unit | ok_reply | abort_reply |
|------|-------|--------|------|----------|-------------|
| SET | 无/NX/XX | 可选 | 秒/毫秒 | NULL(+OK) | NULL($-1) |
| SETNX | NX | 无 | — | shared.cone(1) | shared.czero(0) |
| SETEX | 无 | 必填 | 秒 | NULL | NULL |
| PSETEX | 无 | 必填 | 毫秒 | NULL | NULL |

### 三个参数分别参数化什么

**1. `flags`:NX/XX 语义(位掩码)**

```c
#define REDIS_SET_NX (1<<0)   /* key 不存在才设 */
#define REDIS_SET_XX (1<<1)   /* key 存在才设 */
```

位掩码表达布尔选项,方便以后加新选项。检查逻辑:

```c
if ((flags & REDIS_SET_NX && lookupKeyWrite(c->db,key) != NULL) ||
    (flags & REDIS_SET_XX && lookupKeyWrite(c->db,key) == NULL)) {
    addReply(c, abort_reply ? abort_reply : shared.nullbulk);
    return;
}
```

**2. `expire` + `unit`:过期时间**

统一换算成毫秒保存。SETEX/PSETEX 一定有 expire;SET 只在带 EX/PX 时才有;SETNX 传 NULL。

**3. `ok_reply` / `abort_reply`:回复内容**

SET/SETEX 成功回 `+OK`、失败回 `$-1`;SETNX 成功回 `1`、失败回 `0`。传 NULL 用默认 `shared.ok` / `shared.nullbulk`。

### 命令函数因此很薄

```c
void setnxCommand(redisClient *c) {
    c->argv[2] = tryObjectEncoding(c->argv[2]);
    setGenericCommand(c, REDIS_SET_NX, c->argv[1], c->argv[2],
                      NULL, 0, shared.cone, shared.czero);
}
```

setCommand 较长,因为它要先遍历 argv 解析可变的 NX/XX/EX/PX 选项(大小写不敏感,`a[2] == '\0'` 防 "nxabc" 被误认)。

### 价值

消除重复、语义集中(判断逻辑只有一份)、易扩展。

---

## 三、incrDecrCommand 的溢出检查

`value += incr` 是 `long long` 加法,signed 溢出是**未定义行为(UB)**,必须加法前判断:

```c
oldvalue = value;
if ((incr < 0 && oldvalue < 0 && incr < (LLONG_MIN-oldvalue)) ||
    (incr > 0 && oldvalue > 0 && incr > (LLONG_MAX-oldvalue))) {
    addReplyError(c,"increment or decrement would overflow");
    return;
}
value += incr;
```

### 两个条件分别防什么

1. **负 + 负(下溢)**:`incr < 0 && oldvalue < 0 && incr < (LLONG_MIN-oldvalue)`
   - `value + incr < LLONG_MIN` 移项 → `incr < LLONG_MIN - value`
2. **正 + 正(上溢)**:`incr > 0 && oldvalue > 0 && incr > (LLONG_MAX-oldvalue)`
   - `value + incr > LLONG_MAX` 移项 → `incr > LLONG_MAX - value`

### 为什么只查这两种

相加溢出只有两种情况:正+正超上界、负+负超下界。一正一负结果绝对值只会变小,绝不溢出。前置符号判断做快速排除。

### 为什么用移项而不用 `value + incr < MIN`

若直接写 `value + incr < LLONG_MIN`,`value + incr` 本身已溢出,是 UB。移项成 `incr < LLONG_MIN - value`,右边是两个已知量相减,不产生中间溢出值。

### 对照 INCRBYFLOAT

浮点无 UB 溢出(变 inf/nan),所以 `incrbyfloatCommand` 是"先加再查结果是不是 inf/nan":

```c
value += incr;
if (isnan(value) || isinf(value)) { ... }
```

**整数事前防溢出,浮点事后查无穷。**

---

## 四、setrangeCommand 的字符串扩写与 dbUnshareStringValue

`SETRANGE key offset value`:把 value 覆盖到 key 字符串 offset 位置,超出的中间用 `\0` 补。

### 分支结构

- key 不存在:新建空字符串,检查总长
- key 存在:检查类型、取原长、空串处理,然后 **`dbUnshareStringValue`**

### 核心:dbUnshareStringValue 为什么要先被调用

实际写操作是原地修改:

```c
o->ptr = sdsgrowzero(o->ptr, offset+sdslen(value));  // 扩展补\0
memcpy((char*)o->ptr+offset, value, sdslen(value));   // 覆盖
```

若对象 refcount > 1(被共享),原地改会连累所有共享者。最典型的共享是 `shared.integers`(0~9999 全库唯一)。

### dbUnshareStringValue 内部(db.c 340 行)

```c
robj *dbUnshareStringValue(redisDb *db, robj *key, robj *o) {
    if (o->refcount != 1 || o->encoding != REDIS_ENCODING_RAW) {
        robj *decoded = getDecodedObject(o);
        o = createRawStringObject(decoded->ptr, sdslen(decoded->ptr));
        decrRefCount(decoded);
        dbOverwrite(db,key,o);
    }
    return o;
}
```

触发复制的两个条件:

1. **`refcount != 1`** —— 被共享
2. **`encoding != RAW`** —— 非 RAW 编码

第 2 个条件的深层原因:INT 编码的 ptr 存的是整数(不是 sds 结构)、EMBSTR 是只读内嵌,都**不能原地扩写**。需要一份真正可 realloc 的 RAW sds。

`getDecodedObject` + `createRawStringObject` 配合:先解码成字符串形式,再新建独立 RAW 副本。

### 与之前概念的呼应

| 之前学的 | 这里体现 |
|---------|---------|
| refcount 共享对象不能原地改 | 检查 `refcount != 1` |
| INT 编码 ptr 存整数(非 sds) | 必须 `getDecodedObject` 解码 |
| EMBSTR 不可修改 | `encoding != RAW` 也要复制 |

`dbUnshareStringValue` 是 `tryObjectEncoding` 的镜像:省时抠到极致,真要改动时先把"独占副本"放在第一位。

---

## 五、incrbyfloatCommand 的复制/AOF 一致性处理

函数主体的最后 5 行(574~577 行)是精华:

```c
/* Always replicate INCRBYFLOAT as a SET command with the final value. */
aux = createStringObject("SET",3);
rewriteClientCommandArgument(c,0,aux);
decrRefCount(aux);
rewriteClientCommandArgument(c,2,new);
```

### 背景:主从复制和 AOF 传播的是"命令"而非"结果"

`INCRBYFLOAT k 0.1` 会原样发给从库、写进 AOF,从库/恢复时自己再算一遍。

### 问题:浮点运算结果可能各算各的不同

浮点非精确,不同机器/编译器/libc 对同一浮点计算可能产生不同结果:

- 主库算 `k + 0.1` → `3.3000000000000001`
- 从库算 → `3.3`
- AOF 恢复 → `3.30000000000000001`

差异会累积成真实不一致。

### Redis 解法:传播"结果"而非"命令"

把 `INCRBYFLOAT k 0.1` 重写成 `SET k "3.3"`,让从库/AOF 直接 SET 主库算好的最终值,不自己做浮点运算:

```c
rewriteClientCommandArgument(c,0,aux);   // argv[0] "INCRBYFLOAT" -> "SET"
rewriteClientCommandArgument(c,2,new);   // argv[2] 增量 -> 最终值
```

改写后传播的是确定性命令 `SET k "3.3"`,主库、从库、AOF 恢复后严格一致。

`aux` 是临时造的 "SET" 字符串,替换进 argv[0] 后(可能 incrRefCount)不再需要,`decrRefCount` 释放。

### 设计哲学

分布式系统原则:**传播确定性结果,而非非确定性操作**。浮点运算、时间戳、随机数、locale 排序都有"各机器不同"的风险,稳妥做法是主库算一次,广播唯一确定的结果。
