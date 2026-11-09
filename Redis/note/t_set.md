# t_set.c 模块问答

> 对应源码:`src/t_set.c`
> 相关文件:`src/intset.c`、`src/dict.c`、`src/db.c`

---

## 一、t_set.c 是什么?

实现 Set 类型(key → 无序不重复元素集合)。结构和 t_hash/t_list 一脉相承,分两部分:

1. **上半部分(36~410 行):Set API 层** —— 一组 `setTypeXxx` 多态函数抽象两种编码差异。
2. **下半部分(412 行起):具体命令** —— SADD/SREM/SMOVE/SISMEMBER/SCARD/SPOP/SRANDMEMBER/SINTER/SUNION/SDIFF/SSCAN。

### 两种底层编码

| 编码 | 底层结构 | 适用 |
|------|---------|------|
| INTSET | intset | 全为整数的小集合(≤ `set-max-intset-entries`,默认 512) |
| HT | dict | 含非整数元素,或整数元素过多 |

相比 List/Hash 的两个阈值(数量 + 单元素长度),Set 只有一个数量阈值——intset 只存整数,天然排除"元素过长"问题。

### 与 List/Hash 最大的差异

Set 的 intset/HT 切换不是"元素太多才升级",而是**一旦出现非整数元素立即升级**——intset 结构上完全装不下非整数,不是性能退化,是根本无法存储。

---

## 二、setTypeAdd 的双分支:遇到非整数元素就立即升级

```c
int setTypeAdd(robj *subject, robj *value) {
    if (subject->encoding == REDIS_ENCODING_HT) {
        if (dictAdd(subject->ptr,value,NULL) == DICT_OK) {
            incrRefCount(value);
            return 1;
        }
    } else if (subject->encoding == REDIS_ENCODING_INTSET) {
        if (isObjectRepresentableAsLongLong(value,&llval) == REDIS_OK) {
            uint8_t success = 0;
            subject->ptr = intsetAdd(subject->ptr,llval,&success);
            if (success) {
                if (intsetLen(subject->ptr) > server.set_max_intset_entries)
                    setTypeConvert(subject,REDIS_ENCODING_HT);
                return 1;
            }
        } else {
            // 值不能编码成整数 → 必须先升级成 HT
            setTypeConvert(subject,REDIS_ENCODING_HT);
            redisAssertWithInfo(NULL,value,dictAdd(subject->ptr,value,NULL) == DICT_OK);
            incrRefCount(value);
            return 1;
        }
    } else {
        redisPanic("Unknown set encoding");
    }
    return 0;
}
```

### 三个关键点

**① 升级有两个独立触发条件(与 List/Hash 只靠数量不同)**

1. **数量超阈值**:整数元素太多(> 512),`setTypeConvert` 升级。
2. **遇到非整数元素**:intset 只能存 int64_t,装不下 `"abc"`。`isObjectRepresentableAsLongLong` 判不过,必须全量搬迁成 dict 再加。

List/Hash 的 ziplist 虽限 64 字节,但至少能装下字符串(超长才升级);intset 是结构上的硬约束,只能"装不下立刻升级"。

**② 升级后 redisAssertWithInfo 断言 dictAdd 必成功**

```c
setTypeConvert(subject,REDIS_ENCODING_HT);
redisAssertWithInfo(NULL,value,dictAdd(subject->ptr,value,NULL) == DICT_OK);
```

value 是非整数,而 intset 里原来全是整数,新 value 不可能与旧元素重复,所以 dictAdd 必返回 DICT_OK。这是对本函数内部不变量的断言。

**③ 返回值语义**:dictAdd 返回 DICT_OK(1 新增)/ DICT_ERR(0 已存在),setTypeAdd 沿用,命令层靠它统计 added 数量。

### 与 tryObjectEncoding 的闭环

`saddCommand` 里先 `c->argv[j] = tryObjectEncoding(c->argv[j])`,把 `"123"` 转成共享 INT 对象,之后 `setTypeAdd` 里 `isObjectRepresentableAsLongLong` 直接命中,`intsetAdd` 存的就是那个整数。

---

## 三、setTypeNext 的 COW 友好设计 vs setTypeNextObject

### setTypeNext:COW 友好,返回的对象不加 refcount

```c
int setTypeNext(setTypeIterator *si, robj **objele, int64_t *llele) {
    if (si->encoding == REDIS_ENCODING_HT) {
        dictEntry *de = dictNext(si->di);
        if (de == NULL) return -1;
        *objele = dictGetKey(de);              // 直接返回已存在的对象指针
    } else if (si->encoding == REDIS_ENCODING_INTSET) {
        if (!intsetGet(si->subject->ptr,si->ii++,llele))
            return -1;
    }
    return si->encoding;
}
```

函数注释明确:返回的对象 refcount 不 +1,所以对写时复制(COW)友好。

### setTypeNextObject:非 COW 友好,加工成统一 robj*

```c
robj *setTypeNextObject(setTypeIterator *si) {
    int64_t intele; robj *objele; int encoding;
    encoding = setTypeNext(si,&objele,&intele);
    switch(encoding) {
        case -1: return NULL;
        case REDIS_ENCODING_INTSET: return createStringObjectFromLongLong(intele);
        case REDIS_ENCODING_HT: incrRefCount(objele); return objele;
        default: redisPanic("Unsupported encoding");
    }
    return NULL;
}
```

把两种编码统一成一个 robj* 出口,调用者拿到的一定是"归你所有"的对象,用完必须 decrRefCount。

### 为什么分两个函数 —— COW 的本质

`incrRefCount(o)` 本质是 `o->refcount++`,即一次内存写。fork(BGSAVE/BGREWRITEAOF)后父子进程共享内存页,**任何写操作都会触发 OS 按页(4KB)复制内存**,这就是 COW。

| 函数 | refcount | 适用场景 |
|------|----------|---------|
| `setTypeNext` | 不动 | 只读遍历(SINTER 判断、SRANDMEMBER 只取不删),避免无谓 COW |
| `setTypeNextObject` | +1 | 写操作(SUNION、SMOVE),反正要动对象,顺手拿所有权 |

只读场景连 refcount++ 这种"写"都不做,保证 fork 出的子进程和父进程继续共享内存。

---

## 四、SPOP 的 SREM 改写:随机命令的复制确定性

SPOP 是"随机弹出一个元素",而随机是复制的天敌。

### 问题:随机结果各算各的

主从复制/AOF 传播的是命令。若把 `SPOP myset` 原样发给从库,从库自己再随机弹一次,主库弹 "a"、从库可能弹 "b"、AOF 恢复又弹 "c",集合内容立刻分叉。

### 解法:把「随机弹」重写成「明确删主库弹出的那个」

```c
// 随机取元素 → 删 → 改写传播
aux = createStringObject("SREM",4);
rewriteClientCommandVector(c,3,aux,c->argv[1],ele);   // SPOP key → SREM key ele
decrRefCount(ele);
decrRefCount(aux);
```

主库把"弹出的哪个元素"已决定的事实确定化,广播 `SREM key ele`,从库删的是同一个元素。

### rewriteClientCommandVector vs rewriteClientCommandArgument

- 前面 HINCRBYFLOAT/INCRBYFLOAT 用 `rewriteClientCommandArgument` **逐个替换**参数;
- 这里用 `rewriteClientCommandVector` **整体重写整个 argv 数组**,参数个数从 key 变成 key+ele。

### 引用计数编排

```c
incrRefCount(ele);          // hold 住,防止删时被释放
setTypeRemove(set,ele);     // 从集合删(内部 decrRefCount)
...
addReplyBulk(c,ele);         // 用 ele 回复
decrRefCount(ele);           // 回复+传播完成后再释放
```

### 横向对比

| 命令 | 非确定因素 | 改写为 | 使用的函数 |
|------|-----------|--------|-----------|
| INCRBYFLOAT | 浮点精度 | SET key 最终值 | rewriteClientCommandArgument |
| HINCRBYFLOAT | 浮点精度 | HSET key field 值 | rewriteClientCommandArgument |
| SPOP | 随机选择 | SREM key 弹出的元素 | rewriteClientCommandVector |

共同原则:**传播确定性结果,而非非确定性操作**。

---

## 五、SRANDMEMBER 的四种情形分派

`SRANDMEMBER key [count]` 随机取元素但不删除。复杂度在 count 参数上。

### count 语义

```c
if (l >= 0) { count = l; uniq = 1; }  // 正数:互不相同
else        { count = -l; uniq = 0; } // 负数:可有重复
```

### 四种情形

**情形 1:count 为负(允许重复)** —— 最简单

循环 N 次独立随机取,直接回复,不管重复,不需额外结构。

**情形 2:count >= size(要的比集合还多)** —— 直接返回整个集合

```c
if (count >= size) {
    sunionDiffGenericCommand(c,c->argv+1,1,NULL,REDIS_OP_UNION);
    return;
}
```

单集合的并集就是它本身,复用 SUNION 逻辑。

**情形 3:count 较大(3×count > size)** —— 全量复制后减删

```c
if (count*SRANDMEMBER_SUB_STRATEGY_MUL > size) {  // MUL == 3
    // 把集合全部元素拷贝进临时字典
    // 随机删除到只剩 count 个
    while(size > count) {
        de = dictGetRandomKey(d);
        dictDelete(d,dictGetKey(de));
        size--;
    }
}
```

反直觉做法:要取 90/100,直接随机取会频繁撞重;反着删到 90 只需删 10 个,便宜得多。

**情形 4:count 较小(3×count <= size)** —— 直接随机取 + 去重

```c
else {
    while(added < count) {
        encoding = setTypeRandomElement(set,&ele,&llele);
        if (dictAdd(d,ele,NULL) == DICT_OK) added++;  // 成功才计数
        else decrRefCount(ele);                        // 撞重丢弃
    }
}
```

集合很大只要取少量,撞重概率极低,随机取 + dict 去重即可。

### 核心权衡

| 策略 | 适用 | 原理 |
|------|------|------|
| 随机取 + 去重 | count 远小于 size | 撞重概率低 |
| 全量拷 + 删 | count 接近 size | 删少量比碰撞取大量便宜 |

分界点 `SRANDMEMBER_SUB_STRATEGY_MUL = 3`:count > size/3 时切换。这是"按工作量运行时自适应"的思路,与 SDIFF 的两算法选择同源。

---

## 六、SDIFF 的两算法选择

`sunionDiffGenericCommand` 里,先估算两种差集算法的工作量,挑便宜的走。

### 两个算法(以 SDIFF s1 s2 s3 为例)

- **算法 1**:遍历 s1 每个元素,逐个去 s2、s3 查。复杂度 O(N×M),N=s1 基数,M=其他集合数。
- **算法 2**:把 s1 全塞进结果集,再遍历 s2、s3 删命中元素。复杂度 O(N),N=所有集合元素总数。

### 选择逻辑:估算工作量

```c
for (j = 0; j < setnum; j++) {
    if (sets[j] == NULL) continue;
    algo_one_work += setTypeSize(sets[0]);   // 每个其他集合都扫一遍 s1
    algo_two_work += setTypeSize(sets[j]);   // 所有集合元素累加
}
algo_one_work /= 2;                          // 算法1常数低,打对折
diff_algo = (algo_one_work <= algo_two_work) ? 1 : 2;
```

`algo_one_work /= 2`:算法 1 常数低(纯 dict 查找,集合间有重复时会提前 break),故意打折增加竞争力。

### 选算法 1 时的排序优化

```c
if (diff_algo == 1 && setnum > 1) {
    qsort(sets+1,setnum-1,sizeof(robj*),qsortCompareSetsByRevCardinality);
}
```

把 s1 之外的集合按基数从大到小排,大集合更易命中,能更早 break。

### 选算法 2 的提前退出

```c
if (setTypeRemove(dstset,ele)) cardinality--;
...
if (cardinality == 0) break;   // 结果集已空,再删白删
```

### 与 SRANDMEMBER 的呼应

两者都是"输入到来时估算各算法工作量,选成本低的"——运行时自适应,而非写死某一种算法。

---

## 七、STORE 与非 STORE 版复用:一条泛型函数撑起多条命令

### 六条命令 → 两颗函数

| 运算 | 非 STORE 版 | STORE 版 |
|------|------------|----------|
| 交集 | SINTER | SINTERSTORE |
| 并集 | SUNION | SUNIONSTORE |
| 差集 | SDIFF | SDIFFSTORE |

- `sinterGenericCommand` 撑起 SINTER + SINTERSTORE
- `sunionDiffGenericCommand` 撑起 SUNION/SUNIONSTORE/SDIFF/SDIFFSTORE

### 差异收敛成 dstkey + op 两个参数

```c
void sinterCommand(redisClient *c) {
    sinterGenericCommand(c, c->argv+1, c->argc-1, NULL);
}
void sinterstoreCommand(redisClient *c) {
    sinterGenericCommand(c, c->argv+2, c->argc-2, c->argv[1]);  // dstkey = argv[1]
}
```

- `dstkey == NULL` → 非 STORE,结果直接回复;
- `dstkey != NULL` → STORE,结果存进 argv[1]。

`sunionDiffGenericCommand` 再用 `op` 参数(`REDIS_OP_UNION`/`REDIS_OP_DIFF`/`REDIS_OP_INTER`)区分别并集/差集/交集。

### 两处分叉

**① 取 key 用读还是写**

```c
robj *setobj = dstkey ?
    lookupKeyWrite(c->db,setkeys[j]) :   // STORE:写查找
    lookupKeyRead(c->db,setkeys[j]);     // 非 STORE:只读查找
```

**② 结果回客户端还是存 key**

非 STORE 版 addReply 逐个回复(结尾 decrRefCount 释放临时集合);

STORE 版 `dbDelete` 删旧 dest → 非空才 `dbAdd` 入库 + 发通知 → 空结果 `decrRefCount` 不留 key。

注意两个细节:t_list/t_hash 都见过的 **空结果不留空 key**,以及 `signalModifiedKey` + `server.dirty` 只在真改动时触发。

### 附带小技巧:延迟回填回复长度

```c
if (!dstkey) replylen = addDeferredMultiBulkLength(c);  // 占位
...
setDeferredMultiBulkLength(c,replylen,cardinality);      // 算完回填
```

交集结果元素数要算完才知道,multi-bulk 头先占位最后回填。"结果长度事先未知"的标准做法。
