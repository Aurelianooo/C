# db.c 模块问答

> 对应源码:`src/db.c`
> 相关文件:`src/redis.c`(activeExpireCycle)、`src/dict.c`(dictScan)、`src/object.c`(getDecodedObject)

---

## 一、lookupKey 三兄弟:分层与各自职责

### db.c 的定位

db.c 不是"最顶层"。真正的顶层是 `redis.c`(服务器主循环)和 `networking.c`(协议解析),它们收包 → 解析 → 找到命令 → 才调用到具体实现。db.c 在**类型命令(t_*.c)之下、服务器之上**,是"数据库整体层面"的操作层:

```
redis.c / networking.c      最顶层:收包、解析、分派命令
        ↓
t_string.c / t_set.c ...    类型层:知道"值是字符串还是集合"
        ↓
db.c                        数据库层:不关心值类型,只管 key→value 的增删查
        ↓
dict.c / sds.c ...          底层数据结构
```

核心:**db.c 对 value 是黑盒的**。它知道 key 怎么映射到 value、怎么删、怎么判断过期,但不知道 value 是 set 还是 zset——那是 t_*.c 的事。

### redisDb 的两个 dict

```c
typedef struct redisDb {
    dict *dict;          // 键空间:key → value(redisObject*)
    dict *expires;       // 过期表:key → 过期时间戳(毫秒,裸整数)
    ...
} redisDb;
```

- `db->dict`:所有键值对。
- `db->expires`:**只存设了过期时间的 key**,且 key 与 db->dict **共享同一块 sds**。

### 三个函数的真实代码

```c
robj *lookupKey(redisDb *db, robj *key) {
    dictEntry *de = dictFind(db->dict,key->ptr);
    if (de) {
        robj *val = dictGetVal(de);
        // 只在无子进程时更新 LRU,防止 COW
        if (server.rdb_child_pid == -1 && server.aof_child_pid == -1)
            val->lru = LRU_CLOCK();
        return val;
    } else {
        return NULL;
    }
}

robj *lookupKeyRead(redisDb *db, robj *key) {
    robj *val;
    expireIfNeeded(db,key);          // ① 先处理过期
    val = lookupKey(db,key);         // ② 再查
    if (val == NULL) server.stat_keyspace_misses++;
    else             server.stat_keyspace_hits++;
    return val;
}

robj *lookupKeyWrite(redisDb *db, robj *key) {
    expireIfNeeded(db,key);          // ① 先处理过期
    return lookupKey(db,key);        // ② 再查,不统计
}
```

### 三者的真实区别

| | expireIfNeeded | 更新 LRU | hit/miss 统计 |
|---|---|---|---|
| `lookupKey` | ✗ | ✓(无子进程时) | ✗ |
| `lookupKeyRead` | ✓ | ✓(经 lookupKey) | ✓ |
| `lookupKeyWrite` | ✓ | ✓(经 lookupKey) | ✗ |

容易讲错的三点(已按源码修正):

1. **过期删除不在 `lookupKey` 里**,而在 Read/Write 里各自调 `expireIfNeeded(db,key)`。`lookupKey` 只做"查 dict + 条件更新 LRU"。
2. **`expireIfNeeded` 签名是 `(db, key)`**,内部自己 `getExpire` 取时间戳,不需要 value。
3. **`lookupKeyWrite` 和 Read 的唯一区别是统计**:Read 统计 hit/miss,Write 不统计。两者 LRU 都会更新、过期都会检查。写命令后 value 可能被改,统计命中率意义不大;读命令的命中率反映热数据分布,是 `INFO keyspace` 的来源。

### COW 保护保护的是什么

条件 `server.rdb_child_pid == -1 && server.aof_child_pid == -1` 保护的是 `val->lru` 的**写入**——写 `val->lru` 会触发包含该 val 的 4KB 页复制。所以**任何一次 lookupKey(无论读写)在有 RDB/AOF 子进程时都不更新 LRU**,代价是 LRU 略不准,换来内存安全。

---

## 二、dbAdd / dbOverwrite / setKey

```
setKey  ← 高层,命令层直接调用(SET、SETEX、GETSET…)
  ├─ dbAdd       ← 底层,键不存在:新增
  └─ dbOverwrite ← 底层,键已存在:覆盖
```

### dbAdd(db.c:166)

```c
void dbAdd(redisDb *db, robj *key, robj *val) {
    sds copy = sdsdup(key->ptr);                    // ① 复制键名
    int retval = dictAdd(db->dict, copy, val);      // ② 插入
    redisAssertWithInfo(NULL,key,retval == REDIS_OK);  // ③ 键必须不存在
    if (server.cluster_enabled) slotToKeyAdd(key);
}
```

- **`sdsdup` 复制键名**:新增时主字典还没有 key,必须复制一份长期 sds,不能借调用者的 argv。
- **断言键必须不存在**:`dictAdd` 只在 key 不存在时成功,所以只能用于新增。
- **不加 val 引用计数**:谁调用谁负责加。

### dbOverwrite(db.c:196)

```c
void dbOverwrite(redisDb *db, robj *key, robj *val) {
    dictEntry *de = dictFind(db->dict,key->ptr);
    redisAssertWithInfo(NULL,key,de != NULL);   // 键必须存在
    dictReplace(db->dict, key->ptr, val);
}
```

- **断言键必须存在**,只能用于覆盖。
- **用 `key->ptr` 而非复制**:dict 里已有同名 key,`dictReplace` 沿用旧 key。
- **同样不增加 val 引用**。
- `dictReplace` 内部释放旧 value(decrRefCount)。
- **不修改过期时间**(注释明说),清过期是 setKey 的事。

### setKey(db.c:222)

```c
void setKey(redisDb *db, robj *key, robj *val) {
    if (lookupKeyWrite(db,key) == NULL) {   // ① 先查(会处理过期)
        dbAdd(db,key,val);
    } else {
        dbOverwrite(db,key,val);
    }
    incrRefCount(val);         // ② 统一加引用
    removeExpire(db,key);      // ③ 移除过期时间
    signalModifiedKey(db,key); // ④ 通知 WATCH 客户端
}
```

四步对应注释里的 1)/2)/3):

1. **先 `lookupKeyWrite` 判断**:用 Write 版而不是 `dictFind`,因为它会先 `expireIfNeeded`——如果 key 刚好过期,这里先删掉,然后走 `dbAdd`。否则给一个已过期的 key 做 `dbOverwrite` 会保留它的过期行为。
2. **`incrRefCount(val)`**:dbAdd/dbOverwrite 自己不加,由 setKey 统一加一次。
3. **`removeExpire`**:SET 是新值,新值没有过期时间,所以清掉旧过期记录。
4. **`signalModifiedKey`**:通知 WATCH 了该 key 的客户端,事务可能被中止。

### 引用计数对照

| 函数 | 加 val 引用? | 谁加 |
|---|---|---|
| `dbAdd` | ✗ | 调用者 |
| `dbOverwrite` | ✗ | 调用者 |
| `setKey` | ✓ | setKey 内部统一加 |

底层只管把指针塞进数据结构,引用计数的账由上层算清楚。

### key 的复制差异

- `dbAdd`:`sdsdup(key->ptr)`——新增,必须复制。
- `dbOverwrite`:直接用 `key->ptr`——已有同名 key,沿用旧的。

这就是 `dbDelete` 里"从 expires 删 entry 不会 free sds,因为它和主字典共享"的原因:两个 dict 的 key 指向同一块 sds。

---

## 三、过期系统:三条路径

Redis 没有定时器,一个设了 `EXPIRE 100` 的 key 不会被"准时"删除。三条路径配合清理:

1. **惰性删除**:访问到时才检查、才删(`expireIfNeeded`)。
2. **定期删除**:后台周期性随机抽样(`activeExpireCycle`)。
3. **从库延迟删除**:从库自己不删,等主库发 DEL(`propagateExpire`)。

### 过期时间存在哪

**不是存在 value 里,而是单独一张表 `db->expires`**(key → 过期时间戳)。

#### setExpire(db.c:1089)

```c
void setExpire(redisDb *db, robj *key, long long when) {
    dictEntry *kde, *de;
    kde = dictFind(db->dict,key->ptr);          // 键必须存在于主字典
    redisAssertWithInfo(NULL,key,kde != NULL);
    // 用主字典里的 key 的 sds 去 expires 表里占位
    de = dictReplaceRaw(db->expires,dictGetKey(kde));
    dictSetSignedIntegerVal(de,when);           // 时间戳直接当整数存 entry value
}
```

两个关键点:

1. **复用主字典的 sds**:`dictReplaceRaw(db->expires, dictGetKey(kde))` 传入的是主字典那个 key 的 sds 指针,**不复制**。所以 db->dict 和 db->expires 的同名 key 共享同一块内存。
2. **过期时间是裸整数**:`dictSetSignedIntegerVal`,塞进 entry 的 `v.s64`,不是 redisObject。

`dictReplaceRaw`(不是 `dictAdd`)因为 EXPIRE 可重复设置,已存在时要更新。

#### getExpire(db.c:1114)

```c
long long getExpire(redisDb *db, robj *key) {
    dictEntry *de;
    if (dictSize(db->expires) == 0 ||
       (de = dictFind(db->expires,key->ptr)) == NULL) return -1;  // 无过期→-1
    redisAssertWithInfo(NULL,key,dictFind(db->dict,key->ptr) != NULL);
    return dictGetSignedIntegerVal(de);
}
```

- 返回 **-1 表示"没有过期时间"**(永久键),和"已过期"是两码事。
- 先判 `dictSize == 0` 快速短路。

#### removeExpire(db.c:1076)

```c
int removeExpire(redisDb *db, robj *key) {
    redisAssertWithInfo(NULL,key,dictFind(db->dict,key->ptr) != NULL);
    return dictDelete(db->expires,key->ptr) == DICT_OK;
}
```

只有当主字典里有这个 key 时才允许移除过期,否则 key 永远无法释放。

### 关键问答:setExpire 为什么用 dictGetKey 而不是 key->ptr

**`key->ptr` 和 `dictGetKey(kde)` 是两块不同的 sds:**

- `key` 是调用者的 `c->argv[1]`,是命令解析时为客户端参数**新建的临时 robj**。
- `dictGetKey(kde)` 是 `db->dict` 里**真正存活的、长期存在的**那个 key 的 sds。

内容相同(`dictFind` 按内容查找),地址不同。

**为什么必须用 `dictGetKey(kde)`:**

1. **正确性——生命期**:`c->argv[1]` 命令结束后被 `decrRefCount` 释放。若 expires 存了 `key->ptr`,命令一结束就是**悬空指针**。
2. **内存——共享**:整个设计让 db->dict 和 db->expires 的同名 key 指向同一块 sds,只存一份。
3. **`dictReplaceRaw` 印证**:它裸用传入指针、不复制不接管所有权,正因为 `dictGetKey(kde)` 本就是主字典的长期 sds。

若用 `key->ptr`:命令结束 → 悬空 → 段错误;或每设一次过期多存一份 key 字符串,和共享设计相悖。

`dbAdd` 里 `sdsdup(key->ptr)` 是反方向同理:新增时主字典还没有 key,必须复制长期 sds。

### 路径一:惰性删除 expireIfNeeded(db.c:1176)

```c
int expireIfNeeded(redisDb *db, robj *key) {
    mstime_t when = getExpire(db,key);
    mstime_t now;
    if (when < 0) return 0;                 // ① 永久键
    if (server.loading) return 0;           // ② 正在载入 RDB/AOF
    now = server.lua_caller ? server.lua_time_start : mstime();  // ③ Lua 时间冻结
    if (server.masterhost != NULL) return now > when;  // ④ 从库:只报告不删
    if (now <= when) return 0;              // ⑤ 未到期
    server.stat_expiredkeys++;
    propagateExpire(db,key);                // ⑥ 传播 DEL
    notifyKeyspaceEvent(REDIS_NOTIFY_EXPIRED,"expired",key,db->id);
    return dbDelete(db,key);                // ⑦ 主库真删
}
```

- **② `server.loading`**:载入期间不删,否则载入后数据集和文件不一致。
- **③ Lua 时间冻结**:`server.lua_time_start` 让"现在"固定为脚本开始时刻。否则同一 key 可能"第一次访问没过期、第二次就过期",导致主从不一致(issue #1525)。
- **④ 从库只报告**:`return now > when` 返回逻辑上是否过期,但**不删**,等主库发显式 DEL。

### 路径二:定期删除 activeExpireCycle(redis.c:857)

在 `serverCron` 里每秒 `server.hz` 次调用。两种模式:SLOW(正常)、FAST(微秒级)。

```c
timelimit = 1000000 * ACTIVE_EXPIRE_CYCLE_SLOW_TIME_PERC / server.hz / 100;  // 25% CPU
for (j = 0; j < dbs_per_call; j++) {
    redisDb *db = server.db + (current_db % server.dbnum);
    current_db++;                          // 记录进度,下次从下一个库开始
    do {
        num = dictSize(db->expires);
        if (num == 0) break;
        if (num && slots > DICT_HT_INITIAL_SIZE && num*100/slots < 1) break;
        if (num > ACTIVE_EXPIRE_CYCLE_LOOKUPS_PER_LOOP) num = ACTIVE_EXPIRE_CYCLE_LOOKUPS_PER_LOOP;
        while (num--) {
            de = dictGetRandomKey(db->expires);         // 随机抽样
            ttl = dictGetSignedIntegerVal(de) - now;
            if (activeExpireCycleTryExpire(db,de,now)) expired++;
        }
        if ((iteration & 0xf) == 0 && (ustime()-start) > timelimit) timelimit_exit = 1;
        if (timelimit_exit) return;
    } while (expired > ACTIVE_EXPIRE_CYCLE_LOOKUPS_PER_LOOP/4);  // 过期比例 > 25% 继续
}
```

设计要点:

1. **随机抽样,不是全扫**:`dictGetRandomKey` 每次挑一个,库再大单次开销也可控。
2. **时间预算 25% CPU**:防止过期清理卡住正常请求。
3. **每 16 次迭代才检查超时**(`iteration & 0xf`):`ustime()` 也有开销,性能/精度权衡。
4. **> 25% 就继续**:一次抽样超 1/4 都过期说明积压严重,继续抽;否则交给惰性删除兜底。
5. **`current_db` 轮转**:多库公平分配。
6. **`avg_ttl` 平滑**:`(db->avg_ttl+avg_ttl)/2`,给 `INFO stats` 用。

### 路径三:propagateExpire(db.c:1149)

```c
void propagateExpire(redisDb *db, robj *key) {
    robj *argv[2];
    argv[0] = shared.del;
    argv[1] = key;
    incrRefCount(argv[0]);
    incrRefCount(argv[1]);
    if (server.aof_state != REDIS_AOF_OFF)
        feedAppendOnlyFile(server.delCommand,db->id,argv,2);  // 写 AOF
    replicationFeedSlaves(server.slaves,db->id,argv,2);       // 发从库
    decrRefCount(argv[0]);
    decrRefCount(argv[1]);
}
```

为什么"主库删了要显式发 DEL":

- **主库集中判断**:从库时钟可能不准、不该自作主张。从库永远不主动删过期键,一切删除由主库以普通 `DEL` 命令混入正常命令流发出。
- **AOF 同理**:重放时若依赖过期时间,`now` 和原始执行时不同,结果不一致。AOF 里记 `DEL`,重放无条件删,保证幂等。
- **命令流顺序**:DEL 在复制流里是普通命令,主从/AOF 按序执行,天然一致。

### 三条路径配合图

```
写命令时
   │
   ▼
setExpire ──→ db->expires 表(共享主字典的 key sds,裸整数存时间戳)

运行中
   ├─ 有人访问 key ──→ lookupKeyRead/Write ──→ expireIfNeeded ──→ 惰性删除 ─┐
   │                                                                       │
   └─ serverCron 每秒 hz 次 ──→ activeExpireCycle ──→ 随机抽样 ──→ 定期删除 ─┤
                                                                           │
                                                              主库:propagateExpire
                                                                   ├─→ DEL 写 AOF
              从库:expireIfNeeded 只返回"是/否"                     └─→ DEL 发从库
                    不删,等上面这条 DEL 到来
```

### 不变式

**expires 里有的 key,db->dict 里一定有**(反向不成立)。`getExpire`/`removeExpire` 的断言在守这条。`dbDelete` 先删 expires 再删 dict,且不 free sds(sds 归主字典)。

---

## 四、dbUnshareStringValue 与 COW

### 它解决什么问题

`SETBIT`/`APPEND`/`SETRANGE` 要**原地修改**字符串。两个前置条件不满足就不能直接改:

1. **`refcount != 1`(被共享)**:原地改会"改到别人头上"。
2. **`encoding != RAW`(非 RAW)**:INT 编码没有 sds 缓冲区可原地改。

### 代码(db.c:340)

```c
robj *dbUnshareStringValue(redisDb *db, robj *key, robj *o) {
    redisAssert(o->type == REDIS_STRING);
    if (o->refcount != 1 || o->encoding != REDIS_ENCODING_RAW) {
        robj *decoded = getDecodedObject(o);                            // ① 拿到 sds 表示
        o = createRawStringObject(decoded->ptr, sdslen(decoded->ptr));  // ② 造新独立 RAW 对象
        decrRefCount(decoded);                                          // ③ 释放临时 decoded
        dbOverwrite(db,key,o);                                          // ④ 覆盖 db 里的旧值
    }
    return o;
}
```

- **① `getDecodedObject`**:统一成"有 sds 的形式"(详见下方真实代码)。
- **② `createRawStringObject`**:复制一份全新 sds,新对象 refcount=1、RAW、独占,可放心原地改。
- **③ `decrRefCount(decoded)`**:还掉 ① 里可能的 +1。
- **④ `dbOverwrite`**:换掉 db 里的旧值,内部释放旧 value。

用 `dbOverwrite` 因为前提是 key 已存在(调用者先 `lookupKeyWrite`)。overwrite 不碰过期时间,符合"改值不动 TTL"。

用法(注释里的模式):

```c
o = lookupKeyWrite(db,key);
if (checkType(c,o,REDIS_STRING)) return;
o = dbUnshareStringValue(db,key,o);
// 此后可以放心 sdscat / 原地改
```

### 关键问答:getDecodedObject 到底怎么解码

**易错点**:INT 分支不是 `createStringObjectFromLongLong`,而是 `ll2string` + `createStringObject`。

真实代码(object.c:617):

```c
robj *getDecodedObject(robj *o) {
    robj *dec;
    if (sdsEncodedObject(o)) {      // ① 已是 RAW 或 EMBSTR
        incrRefCount(o);
        return o;                   // 不复制,原对象 +1
    }
    if (o->type == REDIS_STRING && o->encoding == REDIS_ENCODING_INT) {
        char buf[32];
        ll2string(buf,32,(long)o->ptr);              // ② 整数→文本,写入栈上 buf
        dec = createStringObject(buf,strlen(buf));   // ③ 用 buf 建字符串对象
        return dec;
    } else {
        redisPanic("Unknown encoding type");
    }
}
```

- **① `sdsEncodedObject(o)`**(RAW 或 EMBSTR):不复制,`incrRefCount(o)` 后返回原对象。返回的 `decoded` 可能是原对象本身 +1。
- **② INT**:`ll2string` 格式化到栈上 `char buf[32]`,再 `createStringObject` 建新对象(内部按长度选 EMBSTR/RAW)。

区别对照:

| | 易错说法 | 真实代码 |
|---|---|---|
| 转换 | `createStringObjectFromLongLong` | `ll2string(buf,...)` |
| 建对象 | — | `createStringObject(buf, strlen(buf))` |

`createStringObjectFromLongLong` 会尝试用共享整数对象(0~9999 直接返回 `shared.integers`),但 `getDecodedObject` 不走这条路。

**对 dbUnshareStringValue 的影响**:

- 原 `o` 是 **INT**:`decoded` 是新建的字符串对象(refcount=1,临时),`createRawStringObject` 复制数据后 `decrRefCount` 释放它,生命期只在这几行内。
- 原 `o` 是**共享 RAW/EMBSTR**(refcount>1 才进 if;EMBSTR 也进来,因为 `encoding != RAW`):`decoded` 是原对象 +1,`createRawStringObject` 复制数据(EMBSTR → RAW),之后 decr 减回那 +1,原共享不受影响。

`decoded` 的作用只是:提供一个 `->ptr` 是 sds、可 `sdslen` 的来源,好让 `createRawStringObject` 去复制。它自己的编码可能是 EMBSTR,只读不写,无所谓。

### 两个"copy-on-write"别搞混

| | 触发原因 | 机制 | 在哪 |
|---|---|---|---|
| **进程级 COW** | `fork()` 出 RDB/AOF 子进程 | 操作系统内核:父子共享物理页,任一方写页触发 4KB 页复制 | `lookupKey` 里 `rdb_child_pid == -1` 判断 |
| **对象级"写时复制"** | robj 被别的引用共享 / 编码不对 | Redis 自己:复制新 robj 换到 key 上再改 | `dbUnshareStringValue` |

- 进程级 COW 是内核干的,Redis 只能"避免触发"(不更新 LRU 等)。
- 对象级是应用层干的:先断开共享再原地改,保护的是"别的逻辑引用",跟 fork 无关。

共同思想是"不要在共享的东西上原地改",但一个是 OS 页粒度、一个是 robj 粒度。

### 为什么 refcount==1 且 RAW 时跳过

```c
if (o->refcount != 1 || o->encoding != REDIS_ENCODING_RAW) { ... }
return o;
```

- refcount==1:独占。
- RAW:有独立 sds 缓冲区可原地改。

两个都满足 → 天生可独占原地修改,零开销直接返回。这就是 `APPEND` 等热路径不受拖慢的原因。

### 共享整数字符串的例子

Redis 启动时创建 `shared.integers[0..9999]`(0~9999 的字符串对象),refcount 设很大值,任何地方用都 incrRefCount 共享。对这样的 key 做 `APPEND`:`refcount != 1` 成立 → 进 if → 复制新对象 → 原地改,不污染全局共享对象。

---

## 五、SCAN 游标实现

### 为什么需要游标式遍历

`KEYS *` 是一次性 O(N) 全遍历,阻塞主线程。SCAN 的诉求:

1. **增量遍历**:每次返回一小批。
2. **不保存状态**:不在服务器上为每个客户端存迭代器。
3. **容忍 rehash**:遍历中 dict 可能扩容/缩容,元素会移动槽位。

答案:**完全无状态的游标(cursor)**——把遍历进度编码进一个整数,客户端下次传回来。代价是允许**重复**,但保证**不漏**。

### SCAN / HSCAN / SSCAN / ZSCAN 的区别

它们都是**渐进式遍历**,区别是**遍历的对象不同**:

| 命令 | 遍历什么 | 返回什么 | 一次扫描的对象 |
|---|---|---|---|
| `SCAN cursor` | 整个数据库的所有 key | 一批 key 名 | 库里的键 |
| `HSCAN key cursor` | 哈希对象 | field + value 对 | 这个 hash 的字段 |
| `SSCAN key cursor` | 集合对象 | 一批成员 | 这个 set 的元素 |
| `ZSCAN key cursor` | 有序集合对象 | member + score 对 | 这个 zset 的成员 |

核心区别一句话:

- **`SCAN` 扫"键空间"**——库里有哪几个 key。
- **`HSCAN`/`SSCAN`/`ZSCAN` 扫"某个容器的内部元素"**——某个 key 里的 hash/set/zset 有哪些元素。

参数差异:`SCAN cursor [选项]` 选项从 argv[2] 起;`HSCAN key cursor [选项]` 选项从 argv[3] 起(多个 key 参数)。这对应代码里 `i = (o == NULL) ? 2 : 3`。

`SCAN` 只给顶层 key 名,要拿 hash 内部内容得再 `HSCAN`,是**两层**遍历。

### 两层结构

```
scanGenericCommand (db.c:664)   ← 命令层:解析选项、过滤、回复
        ↓ 循环调用
dictScan (dict.c:1300)          ← 字典层:按游标扫描桶,返回下一个游标
```

### 命令层 scanGenericCommand 四步

1. **Step 1 解析选项**:`COUNT`(每次大约返回多少,默认 10)、`MATCH`(模式过滤)。
2. **Step 2 取元素**:
   - **HT/intset/ziplist/skiplist 的字典部分** → 走 `dictScan`,循环 `while (cursor && listLength(keys) < count)` 攒够 count 或游标归 0。
   - **intset / ziplist 编码的小对象** → **一次性全取出,游标置 0**(db.c:773-793)。这些编码元素少,为不保存状态干脆一次返回完。
3. **Step 3 过滤**:`MATCH` 不匹配删掉;`o==NULL`(遍历整库)时对每个 key 调 `expireIfNeeded`,过期键跳过。
4. **Step 4 回复**:先回游标(字符串),再回这批元素。

### 为什么 COUNT 只是"约等于"

`dictScan` 一次扫的是**一个桶(rehash 时一组桶)**。一个桶可能挂 0 个或很多个元素。`while` 循环是"扫完一个桶,不够 count 就再扫下一个",最终数量可能 > 或 << count。COUNT 是提示,不精确。

### 字典层 dictScan 核心

```c
unsigned long dictScan(dict *d, unsigned long v, dictScanFunction *fn, void *privdata) {
    if (dictSize(d) == 0) return 0;
    if (!dictIsRehashing(d)) {          // A:一个表
        t0 = &d->ht[0];
        m0 = t0->sizemask;
        de = t0->table[v & m0];
        while (de) { fn(privdata, de); de = de->next; }
    } else {                            // B:rehash 中,两个表
        t0 = &d->ht[0]; t1 = &d->ht[1];
        if (t0->size > t1->size) { 交换,t0 小 t1 大 }
        m0 = t0->sizemask; m1 = t1->sizemask;
        de = t0->table[v & m0];                     // 先扫小表桶
        while (de) { fn(privdata, de); de = de->next; }
        do {                                        // 再扫大表展开组
            de = t1->table[v & m1];
            while (de) { fn(privdata, de); de = de->next; }
            v = (((v | m0) + 1) & ~m0) | (v & m0);  // 高位进位,低位不变
        } while (v & (m0 ^ m1));
    }
    v |= ~m0;          // m0 以外高位全置 1
    v = rev(v);        // 位反转
    v++;               // +1(作用在"原游标低位")
    v = rev(v);        // 再反转
    return v;
}
```

### 关键一:为什么按位反转来计数(rev+1+rev)

普通整数 +1 是"低位先变":`000,001,010,011`——高位几乎不变、低位一直在变。

rehash 扩容(16→64)时,元素从旧槽 `abc` 可能分到新槽 `0abc` 或 `1abc`(多一个高位)。若按普通低位优先遍历,扩容后**低位已变过的部分会被重扫**,导致大面积重复甚至漏。

Redis 做法:**反转位序后 +1**,等价于"**高位优先变**":

```
mask=1111(16 桶),反转计数顺序:0000 → 1000 → 0100 → 1100 → 0010 → ...
```

这样遍历完 `0000`(高 1 位=0)再走 `1000`(高 1 位=1)时,高位变化不影响已遍历的低位组合。扩容时已扫过的槽在新表里只对应"新增高位的展开",不会回头重扫。

代码 `rev(v); v++; rev(v);` 就是"反转后 +1 再反转"。

### 关键二:v |= ~m0 干什么

反转前先把 `m0` 以上高位全填 1。反转后这些位变成低位,`+1` 从它们开始进位——全为 1 保证进位正确传导到 `m0` 覆盖的真实游标位,同时不影响高位语义。这是"只在有效位上加一"的技巧。

### 关键三:rehash 时为什么扫"小表一个桶 + 大表一组展开桶"

rehash 中元素可能在 ht[0](小而旧),也可能在 ht[1](大而新)。同一逻辑位置:

- 小表里是桶 `v & m0`。
- 大表里是展开后的所有位置 `v & m1`,以及高位加 1 的组合(`do-while` 里 `v = (((v|m0)+1) & ~m0) | (v & m0)` 循环枚举)。

扫小表桶覆盖"还没迁走的",扫大表展开组覆盖"已迁走的"。`while (v & (m0 ^ m1))` 保证把 m0/m1 差异的高位全枚举一遍。

效果:**不管 rehash 进行到哪个阶段都能保证每个元素至少被扫到一次**(可能重复)。

### 关键四:允许重复、保证不漏

- **可能重复**:rehash 时元素可能在 ht[0] 和 ht[1] 展开组各被扫一次;遍历中新写入的元素可能落在没扫到的位置。
- **保证不漏**:无状态游标依赖"桶索引遍历顺序",reverse binary iteration 保证"已扫过的低位组合不当成新位置";rehash 时"小表桶 + 大表展开组"覆盖迁移前后两处。

### 直观例子

8 桶(mask=111),反转遍历顺序:

```
000 → 100 → 010 → 110 → 001 → 101 → 011 → 111 → 0(结束)
```

对比普通顺序 `000→001→010→011→100→101→110→111`。反转顺序"高位优先",已扫的高位组合在扩容后能准确知道哪些低位展开还没扫。

### scanCallback 的类型无关提取(db.c:593)

`dictScan` 只负责按游标把桶里的 entry 交给回调,取什么由回调决定:

| o 是什么 | 取出什么 |
|---|---|
| `NULL`(扫整库) | 复制 key 的 sds 造新字符串对象(不 incrRefCount,库的 key 是 sds 不是 robj) |
| Set | `dictGetKey` + incrRefCount |
| Hash | key + val 都 +1 |
| ZSet | key +1,val 是 `double*`,转成字符串对象 |

这就是同一函数服务 SCAN/HSCAN/SSCAN/ZSCAN 的原因——**遍历逻辑共享,元素提取靠回调区分类型**。

### SCAN 一句话总结

用**无状态的反转位序游标**:dictScan 每次按游标扫一个桶(rehash 时扫"小表桶 + 大表展开组"),返回下一个游标;`rev+1+rev` 保证高位优先遍历不漏;代价是可能重复,COUNT 只是提示。命令层负责选项解析、MATCH/过期过滤、多类型提取。

---

## 六、key 提取 API 与集群槽位 API

db.c 末尾有两个**完全不相干**的模块:

1. **key 提取 API**(db.c:1412-1560):从命令参数里解析出哪些位置是 key。
2. **集群槽位 API**(db.c:1562-1676):把 key 按 hash slot 归组,存在 skiplist 里。

### 一、key 提取 API

#### 为什么需要"从命令里找出 key"

命令真正执行前经常需要知道"这条命令碰哪些 key":

- **集群模式**:转发到持有对应 slot 的节点,得先知道涉及哪些 key、是否同 slot。
- **事务 WATCH / Lua 脚本**:提前检查/锁定 key。
- **阻塞命令**:知道 key 释放时唤醒谁。

难点:不同命令 key 位置不同,有些是**参数动态决定**的:

```
SET k v                       key 在 argv[1]
ZUNIONSTORE dest 2 k1 k2      key 有 dest(argv[1]) + k1 k2(argv[3..4])
EVAL script 2 k1 k2 arg       key 个数由 argv[2] 决定
SORT k ... STORE dest         主 key argv[1],STORE 的 dest 藏在选项
```

#### getKeysFromCommand(db.c:1448)

```c
int *getKeysFromCommand(struct redisCommand *cmd, robj **argv, int argc, int *numkeys) {
    if (cmd->getkeys_proc)
        return cmd->getkeys_proc(cmd,argv,argc,numkeys);
    else
        return getKeysUsingCommandTable(cmd,argv,argc,numkeys);
}
```

返回**堆分配的 int 数组**,元素是"第几个参数是 key"的**下标**;`*numkeys` 是数量。用完 `getKeysFreeResult` 释放。

#### getKeysUsingCommandTable(db.c:1418)

命令表里每个命令记录三元组 `firstkey / lastkey / keystep`:

```c
if (cmd->firstkey == 0) { *numkeys = 0; return NULL; }   // 无 key 的命令(PING)
last = cmd->lastkey;
if (last < 0) last = argc + last;                        // 负数=倒数
for (j = cmd->firstkey; j <= last; j += cmd->keystep)    // 按步长扫
    keys[i++] = j;
```

如 `MSET k1 v1 k2 v2`:`firstkey=1, lastkey=-1, keystep=2` → 下标 1、3,跳过 value。`lastkey` 为负表示"从末尾倒数"。

只对"key 位置固定/有规律"的命令有效。位置不规则的写专门 `getkeys_proc`。

#### 三个专用 getkeys 函数

1. **zunionInterGetKeys**(db.c:1464):`ZUNIONSTORE dest num k1 k2...`
   - `num = atoi(argv[2])`,校验 `num > argc-3` 防越界。
   - 数据 key 在 `argv[3..3+num-1]`,**dest 在 argv[1]**,所以先填数据 key,最后 `keys[num]=1`。

2. **evalGetKeys**(db.c:1493):`EVAL script num k1 k2...`
   - key 在 `argv[3..3+num-1]`,没有额外 dest。

3. **sortGetKeys**(db.c:1521):`SORT key ... STORE dest`
   - `argv[1]` 永远是 key。
   - 扫描选项找 STORE:用 `skiplist[]` 表记 `LIMIT(跳2)/GET(跳1)/BY(跳1)`,遇到就按 skip 跳过参数,避免误判。
   - 找到 STORE 记 `i+1`。注释:不 `num++` 且继续扫完——多个 STORE 以**最后一个**为准(和 SORT 本身一致)。

三类函数共同点:返回"参数下标数组",只是下标按命令语法解析,不是固定规律。

### 二、集群槽位 API

#### slot 是什么

**slot(哈希槽)= 把整个 key 空间切成 16384 份,每份一个 slot,是集群里"数据归属"的最小单位。**

集群分两层映射:

```
key --(CRC16)--> slot(0~16383) --(人为分配)--> 节点
```

1. **key → slot**:`slot = CRC16(key) % 16384`,固定算法,全局一致,不存表。
2. **slot → 节点**:可配置,由集群管理者决定"哪些 slot 归哪个节点",可迁移(扩容/缩容核心)。

例子:3 节点,16384 slot 均分:

```
节点A: slot    0 ~  5460
节点B: slot 5461 ~ 10922
节点C: slot 10923 ~ 16383
```

`SET foo bar` → `CRC16("foo") % 16384 = 12182` → 属节点C。只要分配不变,`foo` 永远在节点C。

为什么 16384:

- 够细粒度分配。
- 节点间心跳同步"谁管哪些 slot"的位图 16384 bit = 2KB,心跳包塞得下;65536 则 8KB,流量翻 4 倍。
- 16384 = 0x4000,`% 16384` 是取 CRC16 低 14 位,极快。

#### 为什么用 skiplist 按 slot 归组

集群迁移某 slot 时要快速列出"属于该 slot 的所有 key",遍历全库 O(N) 太慢。所以维护一张 `server.cluster->slots_to_keys` 的 zskiplist:

- **用 skiplist 不用 dict** 的原因:skiplist 按 score 有序。把 **hashslot 当 score、key 当 member** 存进去,"找出所有属于 slot X 的 key"就变成一次有序区间查询——`zslFirstInRange(slot,slot)` 定位第一个,沿链表走到 score 不等就停。O(logN + M)。

#### 增删(db.c:1568, 1579)

```c
void slotToKeyAdd(robj *key) {
    unsigned int hashslot = keyHashSlot(key->ptr, sdslen(key->ptr));
    zslInsert(server.cluster->slots_to_keys, hashslot, key);
    incrRefCount(key);                              // skiplist 持有引用
}

void slotToKeyDel(robj *key) {
    unsigned int hashslot = keyHashSlot(key->ptr, sdslen(key->ptr));
    zslDelete(server.cluster->slots_to_keys, hashslot, key);
}
```

- `slotToKeyAdd` 在 `dbAdd` 里调用;`slotToKeyDel` 在 `dbDelete` 里调用。
- add 时 incrRefCount(skiplist 节点 obj 也持有),del 时 zslDelete 内部 decrRefCount。

#### 四个查询/操作函数

| 函数 | 作用 | 关键点 |
|---|---|---|
| `getKeysInSlot` | 取某 slot 最多 count 个 key 填进数组 | `zslFirstInRange` + 沿 `level[0].forward` 走 |
| `countKeysInSlot` | 数某 slot 有多少 key | 不遍历,两次 `zslGetRank` 相减 |
| `delKeysInSlot` | 删除某 slot 所有 key | 见下引用计数 |
| `slotToKeyFlush` | 清空整表 | `zslFree` 后重新 `zslCreate` |

#### countKeysInSlot 的区间算法(db.c:1637)

不逐个走:

```c
rank_first = zslGetRank(zsl, 第一个slot元素的score, obj);
count = zsl->length - (rank_first - 1);         // 从第一个到末尾总数
rank_last  = zslGetRank(zsl, 最后一个slot元素的score, obj);
count -= zsl->length - rank_last;               // 减掉末尾不属于该 slot 的
```

即 `count = rank_last - rank_first + 1`,O(logN)。和 t_zset 里 **ZCOUNT 用两次 zslGetRank 相减**同一手法。

#### delKeysInSlot 的引用计数细节(db.c:1616)

```c
while (n && n->score == hashslot) {
    robj *key = n->obj;
    n = n->level[0].forward;      // 先保存下一个节点
    incrRefCount(key);            // 保护,因为 dbDelete 里 slotToKeyDel 会 decr
    dbDelete(&server.db[0], key);
    decrRefCount(key);            // 还掉保护
    j++;
}
```

为什么要 incrRefCount:dbDelete 内部调 slotToKeyDel → zslDelete 节点 → decrRefCount 掉 skiplist 那份引用。若 key 只被 skiplist 持有,dbDelete 一执行 key 就被释放,后面 decrRefCount 会对已释放对象操作。先 +1 保住,删完 -1。这是遍历中删节点的经典手法(同 t_zset 删节点前先取 forward)。

### 两块定位总结

| 模块 | 目的 | 核心数据结构 | 何时用 |
|---|---|---|---|
| **key 提取 API** | 找出命令参数里哪些是 key | 命令表三元组 / 专用 `getkeys_proc` | 命令执行**前**:集群转发、WATCH、阻塞唤醒 |
| **集群槽位 API** | 按 hash slot 管理 key | `slots_to_keys` zskiplist(score=slot) | 集群模式下 key 增删维护、slot 迁移/统计 |

两者都只在集群或命令分发场景才真正生效,普通单机读写几乎不碰,是为 Redis Cluster 和命令分发服务的。
