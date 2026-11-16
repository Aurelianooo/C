# t_zset 模块源码阅读笔记

> Redis 3.0 的有序集合（ZSET）实现。底层用「dict（成员→分值）+ 跳表 zskiplist（按分值排序）」双结构，支持 O(1) 按成员取分值、O(log N) 的排名/范围操作。

---

## 一、核心结构体（redis.h）

```c
typedef struct zskiplistNode {
    robj *obj;                    // 成员对象
    double score;                 // 分值（排序依据）
    struct zskiplistNode *backward;  // 后退指针
    struct zskiplistLevel {
        struct zskiplistNode *forward;  // 前进指针
        unsigned int span;              // 跨度
    } level[];                    // 柔性数组层
} zskiplistNode;

typedef struct zskiplist {
    zskiplistNode *header, *tail;
    unsigned long length;         // 节点数量
    int level;                    // 最高层的层数
} zskiplist;

typedef struct zset {
    dict *dict;                   // 成员→分值，O(1) 取分值
    zskiplist *zsl;               // 按分值排序，O(log N) 范围/排名
} zset;
```

关键宏：`ZSKIPLIST_MAXLEVEL 32`、`ZSKIPLIST_P 0.25`。

---

## 二、为什么要设计分值 score？

跳表是**有序**结构，必须回答"元素谁前谁后"。`score` 就是**排序的关键码**：

- `obj` 是成员本身（要存的东西）；
- `score` 是给成员打的"分"，决定节点在跳表中的位置。

把"排序度量"从"成员对象"里独立出来，让 zset 能通用表达"排行榜、时间线、优先级队列"这类"给成员打一个分、按分排序"的需求。`double` 天然支持整数/小数/负数/时间戳。

score 相同时，按成员的字典序（`obj`）兜底，保证顺序唯一稳定（`score` 主键 + `obj` 次键）。

---

## 三、Redis 既然是大号字典，为啥要 zset？

Redis 顶层确实是 `db->dict`（大号字典），但**字典不维护顺序**，`value` 可以是有结构的类型。

`zset` 解决的是"**有序**"这个字典做不到的需求：排行榜、范围查询、排名。内部用 dict（按成员 O(1) 查分值）+ 跳表（按分值 O(log N) 排序/范围）双结构互补。

选跳表而不是红黑树/排序数组，因为：跳表性能和平衡树相当、但实现简单（无复杂旋转）、利于并发、方便做"按排名"（靠 span）。

---

## 四、`zslInsert` 与 rank 数组

### 函数三部曲

1. **定位**（208-235 行）：从高层到低层，每层用 `while` 前进（先比 score，再比 obj 字典序），边前进边累积 `rank[i]`、记录 `update[i]`（每层前驱）。
2. **处理高度变化**（246-265 行）：随机层数若超过现有最高层，补初始化高层（update 指向 header、span=length）。
3. **插入 + 重算 span**（267-306 行）：新节点接到每层 `update[i]` 后面，用 `rank[0]-rank[i]` 重算 span；未接触层 span++；设置 backward、更新 tail/length。

### rank 数组在干嘛

```c
zskiplistNode *update[ZSKIPLIST_MAXLEVEL];  // 每层前驱（要改指针的对象）
unsigned int rank[ZSKIPLIST_MAXLEVEL];       // 每层走到插入位置的累计跨度
```

- `update[i]`：第 i 层插入位置的前一个节点。
- `rank[i]`：从表头走到第 i 层插入位置，累计跨过的节点数。

**rank 的继承**（216 行）：`rank[i] = (i == zsl->level-1) ? 0 : rank[i+1]`，高层走多远，低层从那里继续，所以一层层累积。

**rank 的使命**：提供 `rank[0]` 和 `rank[i]` 两个值，用 `rank[0] - rank[i]` = "第 i 层 update 到真正插入点之间相隔的节点数"，从而把每一层原来的 forward 边精确切成两段，重算 span：

```c
x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
update[i]->level[i].span = (rank[0] - rank[i]) + 1;
```

**关键洞察**：当各层 `update[i]` 不同（层越高停得越靠前）时，`rank[i]` 之间才有差异，`rank[0]-rank[i]` 非零，span 重算才体现出 rank 的价值；若所有层 update 相同，则差值全 0，退化成简单情形。

---

## 五、Lexicographic ranges（字典序区间）

不按 score、而按**成员名的字符串字典序**取区间的机制。对应 `ZRANGEBYLEX`/`ZREVRANGEBYLEX`/`ZLEXCOUNT`。

适用场景：成员分值都相同（如都 0），想按名字 ABC 顺序取一段。

### 区间语法

- `[` 闭区间（含边界）、`(` 开区间（不含边界）；
- `-` 负无穷、`+` 正无穷。

例：`ZADD s 0 a 0 b 0 c 0 d 0 e` 后，`ZRANGEBYLEX s [b [d` → `b c d`，`(b (d` → `c`。

### 结构体

```c
typedef struct {
    robj *min, *max;       // 可能指向 shared.minstring/maxstring
    int minex, maxex;      // 是否开区间
} zlexrangespec;
```

与按分值区间的 `zrangespec` 是两套。

---

## 六、`zslParseLexRangeItem` 与 incrRefCount

```c
case '+':  *dest = shared.maxstring;  incrRefCount(shared.maxstring);
case '-':  *dest = shared.minstring;  incrRefCount(shared.minstring);
case '(':  *dest = createStringObject(c+1,...);   // refcount=1，不 incr
case '[':  *dest = createStringObject(c+1,...);   // refcount=1，不 incr
```

**为什么 `+`/`-` 要 incrRefCount，`(`/`[` 不用**：

- `+`/`-` 返回的是**全局共享对象** `shared.maxstring/minstring`（可能已被多处引用），持有它必须 `incrRefCount`，与 `zslFreeLexRange` 里的 `decrRefCount` 配对；
- `(`/`[` 用的是 `createStringObject` **新建的独占对象**（refcount 天生为 1），不需要再 incr。

**通用原则**：多一个地方持有 robj，就 incrRefCount；少一个持有者就 decrRefCount；引用计数归 0 才真 free。这是 Redis 内存管理的基石。

---

## 七、shared.minstring / shared.maxstring 是什么

启动时创建、全局唯一的两个 robj（redis.c 1712-1713 行），内容分别是 `"minstring"`/`"maxstring"`，但**内容不重要**。

### 角色：字符串界的 ±∞ 哨兵

`ZRANGEBYLEX` 需要表示"负无穷/正无穷"，Redis 用这两个对象充当 -∞ / +∞。

### 怎么做到"比任何字符串都小/大"

靠**指针身份**而非内容（`compareStringObjectsForLexRange`）：

```c
if (a == shared.minstring || b == shared.maxstring) return -1;  // a 更小
if (a == shared.maxstring || b == shared.minstring) return 1;   // a 更大
```

一方是 `shared.minstring`（指针相等），直接判"极小"；是 `shared.maxstring` 判"极大"。

### 为什么必须"共享且唯一"

1. **省内存/免创建**：`-`/`+` 高频出现，新建+销毁会浪费 CPU 和内存；
2. **身份比较要求指针唯一**（根本原因）：若到处 new 一个 `"minstring"`，`a == shared.minstring` 就比不中，哨兵机制失效。必须全局唯一才能用 `==` 判"这就是无穷哨兵"。

这是 Redis"共享对象"机制的代表（同 `shared.integers[0..9999]`、`shared.crlf`、`shared.ok`），把高频小对象/哨兵对象全局共享，省内存 + 支持身份比较。

---

# 附：t_zset.c 命令层问答（第三阶段 · 排除 zsl 部分）

> 以下是对 t_zset.c 中除 `zsl` 开头函数之外部分的讲解，覆盖 Ordered Set 的命令实现与双结构/双编码的运作。

---

## 八、zset 的本质：为什么 dict + skiplist 一起存

`zset` 结构里同时有 `dict` 和 `zskiplist`，同一个成员存两份。

### 两种需求天然冲突

| 操作 | 例子 | 需要的结构 |
|------|------|-----------|
| 查/改某个成员的分值 | ZSCORE、ZADD 判存在、ZINCRBY | dict（O(1) 按 member 定位） |
| 按分值排序/排名访问 | ZRANGE、ZRANK、ZREVRANGE | skiplist（O(logN) 有序） |

dict 查得快但不保序，skiplist 保序但不按 member 索引。Redis 选择**两个都要**：dict 管"按成员查分值"，skiplist 管"按分值排顺序"。

### 两个结构共享同一个 member 对象

```c
znode = zslInsert(zs->zsl,score,ele);      // skiplist 持有 ele
incrRefCount(ele); /* Inserted in skiplist. */
dictAdd(zs->dict,ele,&znode->score);
incrRefCount(ele); /* Added to dictionary. */   // dict 也持有 ele
```

member 不拷贝，两个结构都引用同一个 robj（refcount 各 +1）。

### 关键：dict 的 value 存的是 score 的「指针」

```c
dictAdd(zs->dict, ele, &znode->score);   // value = score 字段的地址，而非值
```

好处：**一份 score，两个结构共享**。更新分值时"删旧节点 + 插新节点"，新 skiplist 节点有新 score，dict 里 value 指针同步改指新地址，不会出现两份 score 各存各的不一致。

### 一致性约束：增删改必须两头同步

```c
zslDelete(zs->zsl,score,c->argv[j]);   // 从 skiplist 删
dictDelete(zs->dict,c->argv[j]);       // 从 dict 删
```

少删一个就是数据不一致 + 内存泄漏。

### 对比 set

| | set | zset |
|---|-----|------|
| 底层 | intset 或 dict（二选一） | dict + skiplist（同时两个） |
| 为什么 | 只有"集合"一种语义 | 有"成员→分值"和"分值排序"两种语义 |

---

## 九、score 是哪来的

**score 是用户输入的 double，不是 Redis 分配的。**

- `ZADD key score1 member1 score2 member2 ...`，score 由用户在命令里显式给出。
- `zaddGenericCommand` 用 `getDoubleFromObjectOrReply` 把 argv 里的 `2,4,6,...` 位置解析成 double。
- score 存在容器（zskiplistNode 的 score 字段 / ziplist 的 score 节点）里，**不在 robj 上**。robj 的三个字段 type/encoding/ptr 里没有 score。
- 唯一"默认分值"例外：ZUNIONSTORE/ZINTERSTORE 里 set 元素参与聚合时，约定 score = 1.0（因为 set 没有 score）。

---

## 十、zsetConvert 双向转换

zset 是唯一支持**双向**转换的类型（List/Set 只能单向升级）。

### 为什么支持降级

ZUNIONSTORE/ZINTERSTORE 等存储命令的**结果集是现算出来的**，大小取决于本次输入，可能很小：

```c
if (dstzset->zsl->length <= server.zset_max_ziplist_entries &&
    maxelelen <= server.zset_max_ziplist_value)
        zsetConvert(dstobj, REDIS_ENCODING_ZIPLIST);   // 结果小，降级省内存
```

不是"大对象会缩回去"，而是"新算出的结果按实际大小重新选编码"。

### ziplist → skiplist：逐元素搬进两个结构

```c
while (eptr != NULL) {
    score = zzlGetScore(sptr);
    ele = createStringObject...;          // 构造对象
    node = zslInsert(zs->zsl,score,ele); // 插 skiplist
    dictAdd(zs->dict,ele,&node->score);  // 插 dict
    incrRefCount(ele);                    // 只 +1（见下）
    zzlNext(zl,&eptr,&sptr);
}
zfree(zobj->ptr);                          // 释放旧 ziplist
```

### 为什么这里只 incrRefCount 一次

`ele` 是 `createStringObject` **新建**的，初始 refcount=1，这个 1 恰好充当"skiplist 持有的那份"。所以只需再 `+1` 补 dict 那份，凑够 **2**（dict 1 + skiplist 1）。

对比 zadd 新增路径：那里 `ele` 是**外部传入**的（refcount≥1 属于别人），skiplist 和 dict 的持有都必须各显式 `+1`，共 +2。

| 场景 | ele 初始身份 | incr 次数 | 最终 refcount |
|------|------------|----------|--------------|
| zadd 新增 | 外部传入 | +2 | 原值 +2 |
| zsetConvert 转换 | 函数内新建 | +1 | 2 |

### skiplist → ziplist：先释放 dict，再沿第 0 层边拆边压

```c
dictRelease(zs->dict);                  // ① 先释放 dict（不需要它遍历）
node = zs->zsl->header->level[0].forward;
...
while (node) {
    ele = getDecodedObject(node->obj);      // 借出对象
    zl = zzlInsertAt(zl,NULL,ele,node->score);
    decrRefCount(ele);                      // 还掉借出的 +1
    next = node->level[0].forward;
    zslFreeNode(node);                      // 释放节点（内部 decrRefCount node->obj）
    node = next;
}
```

### 为什么这里只 decrRefCount 一次

降级前 member 对象 refcount=2（dict 1 + skiplist 1）。释放的三处：

1. `dictRelease`:通过 dict 的 key 析构函数，对 member `decrRefCount`（减 dict 那份）；
2. `zslFreeNode(node)`:内部 `decrRefCount(node->obj)`（减 skiplist 那份）；
3. `getDecodedObject` 借出时 +1，`decrRefCount(ele)` 还掉。

三份引用（dict / skiplist / ele 临时借）分别在三个地方释放，加起来归零。

---

## 十一、zaddGenericCommand 的删除重插

更新分值不能"原地改 score"，因为 **score 决定节点在 skiplist/ziplist 里的排序位置**，位置变了节点必须挪动。做法是"删旧节点 + 插新节点"。

### ziplist 分支

```c
if (score != curscore) {
    zobj->ptr = zzlDelete(zobj->ptr,eptr);    // 删旧（member+score 两个节点）
    zobj->ptr = zzlInsert(zobj->ptr,ele,score); // 按新 score 插回
}
```

### skiplist 分支的三步

```c
zslDelete(zs->zsl,curscore,curobj);         // ① 删旧
znode = zslInsert(zs->zsl,score,curobj);    // ② 插新
incrRefCount(curobj); /* Re-inserted in skiplist. */
dictGetVal(de) = &znode->score;              // ③ 更新 dict 的 score 指针
```

第 ③ 步是关键：dict value 存的是 `&znode->score`，删除重插后新节点有新地址，必须改指新节点的 score 字段。

### curobj 为什么 incrRefCount

`zslDelete` 会 `zslFreeNode`（对 node->obj decrRefCount），但紧接着又把同一 curobj 插回 skiplist，所以 `+1` 补回。

### NaN 检查（呼应 t_string）

```c
if (incr) {
    score += curscore;
    if (isnan(score)) { addReplyError(c,nanerr); goto cleanup; }
}
```

`+inf + (-inf)` = NaN，浮点累加后检查，跟 `incrbyfloatCommand` 同款。

### 先解析所有 score，再逐个添加（原子性）

```c
scores = zmalloc(sizeof(double)*elements);
for (j = 0; j < elements; j++) {
    if (getDoubleFromObjectOrReply(c,c->argv[2+j*2],&scores[j],NULL) != REDIS_OK)
        goto cleanup;   // 任一 score 非法，整条命令不执行
}
```

先一次解析完所有 score，任何错误直接 `goto cleanup`，保证命令"要么全生效、要么全不生效"（原子性）。

### 数量/长度双重升级阈值

```c
if (zzlLength(zobj->ptr) > server.zset_max_ziplist_entries) zsetConvert(...);
if (sdslen(ele->ptr) > server.zset_max_ziplist_value)   zsetConvert(...);
```

---

## 十二、两套范围体系：ZRANGEBYSCORE vs ZRANGEBYLEX

| 命令 | 按什么排 | 用途 |
|------|---------|------|
| ZRANGEBYSCORE | score 数值区间 | "分值在 80~100" |
| ZRANGEBYLEX | member 字符串字典序 | "member 在 a~z" |

### 符号约定不同

- **score 用 `(`**：`ZRANGEBYSCORE z (1.5 (2.5` → `1.5 < score < 2.5`（两边开）。解析在 `zslParseRange`：开头 `(` 置 `minex/maxex=1`。
- **lex 用 `[` `(` `-` `+`**：`[` 闭、`(` 开、`-` 负无穷、`+` 正无穷。解析在 `zslParseLexRangeItem`，用 `shared.minstring/maxstring` 充当 ±∞。

### 底层：两套平行函数

| 功能 | score 范围 | lex 范围 |
|------|-----------|---------|
| 判断在范围内 | `zslValueGteMin/LteMax` | `zslLexValueGteMin/LteMax` |
| 找第一个/最后一个 | `zslFirstInRange/LastInRange` | `zslFirstInLexRange/LastInLexRange` |
| 删除范围内 | `zslDeleteRangeByScore` | `zslDeleteRangeByLex` |

差别只在**比较器**：score 范围比 double，lex 范围用 `compareStringObjectsForLexRange`（处理 ±∞ sentinel + 字符串字典序）。

### 两种范围结构

```c
typedef struct { double min,max; int minex,maxex; } zrangespec;   // score：存 double
typedef struct { robj *min,*max; int minex,maxex; } zlexrangespec; // lex：存 robj*
```

### ZRANGEBYLEX 的前提

`zslInsert` 的比较规则里，score 相同时继续比 member 字典序（`compareStringObjects`）。因此**所有 member 同分值时，skiplist 里顺序 = member 字典序**，lex 范围查询才成立。

---

## 十三、zsetopsrc 通用迭代器（ZUNIONSTORE/ZINTERSTORE）

ZUNIONSTORE/ZINTERSTORE 的输入 key 可以是 set 也可以是 zset，需要一套统一迭代器屏蔽差异。

### union 装下四种编码

```c
typedef struct {
    robj *subject; int type; int encoding; double weight;
    union {
        union _iterset {   // set
            struct { intset *is; int ii; } is;
            struct { dict *dict; dictIterator *di; dictEntry *de; } ht;
        } set;
        union _iterzset {  // zset
            struct { unsigned char *zl; unsigned char *eptr,*sptr; } zl;
            struct { zset *zs; zskiplistNode *node; } sl;
        } zset;
    } iter;
} zsetopsrc;
```

### 三个函数统一接口

- `zuiInitIterator`：按 type+encoding 初始化对应迭代器字段；
- `zuiNext`：取当前元素到 `zsetopval` 并前移，**set 元素 score 默认 1.0、zset 元素取真 score**；
- `zuiFind`：在来源里查元素，找到则写回 score（呼应第九节"set 元素 score=1.0"）。

### 配套 zsetopval：惰性转换

```c
typedef struct {
    int flags;
    unsigned char _buf[32];
    robj *ele;                    // 对象形式
    unsigned char *estr; unsigned int elen;  // 字符串形式
    long long ell;                // 整数形式
    double score;
} zsetopval;
```

用 `OPVAL_DIRTY_ROBJ/DIRTY_LL/VALID_LL` 标志位记录"已转成哪种形态"，按需转换（`zuiLongLongFromValue`/`zuiObjectFromValue`/`zuiBufferFromValue`）。

### 为什么惰性转换而不是提前转好

元素可能来自 intset（整数）、dict（robj）、ziplist（字节）、skiplist（robj）四种形态，而聚合流程里**往往只需要其中一种**（比整数就只需整数，插 dict 就只需对象）。提前全转三种形态会造成大量无用功（多 malloc、多 refcount、多临时对象）。

惰性转换 = 推迟到"确切知道需要哪种形态"时再转，只转必要的那一种。代价是加 flags 位做"已转过一次"的簿记。

这与前面"能省就省"一脉相承：tryObjectEncoding 省内存、setTypeNext 的 COW 友好、zsetopval 的按需转换。

### 应用流程

```c
src = zcalloc(sizeof(zsetopsrc) * setnum);
// 为每个 key 建 zsetopsrc，解析 WEIGHTS/AGGREGATE
qsort(src,setnum,sizeof(zsetopsrc),zuiCompareByCardinality);  // 按基数排序
// ZINTERSTORE 遍历最小集合 src[0]，zuiNext 取元素、zuiFind 查其它、zunionInterAggregate 聚合
// ZUNIONSTORE 遍历所有集合，zuiFind 去重
```

---

## 十四、ZCOUNT 的巧算：两个 rank 相减

skiplist 编码下不遍历，用 `zslGetRank` 求首尾元素 rank，做差得个数。

### 核心算法

```c
zn = zslFirstInRange(zsl, &range);
if (zn != NULL) {
    rank = zslGetRank(zsl, zn->score, zn->obj);      // 第一个元素 rank
    count = (zsl->length - (rank - 1));               // 从 first 到末尾的个数

    zn = zslLastInRange(zsl, &range);
    if (zn != NULL) {
        rank = zslGetRank(zsl, zn->score, zn->obj);  // 最后一个元素 rank
        count -= (zsl->length - rank);                // 减去范围右端之外的尾巴
    }
}
```

### 数学本质

```
count = [length - (first_rank - 1)] - [length - last_rank]
      = last_rank - first_rank + 1
```

两次 O(logN) 的 rank 查询拼出区间元素个数，而非 O(N) 遍历。

### 为什么能 O(logN)：span 字段

`zslGetRank` 靠累加 `span` 得 rank，这正是 jump 表维护 `span` 的意义之一——让 rank 查询和区间计数能 O(logN) 完成。也解释了 `zslInsert`/`zslDeleteNode` 里为什么大费周章维护 span。

### ziplist 分支只能遍历

```c
while (eptr) {
    if (!zslValueLteMax(score,&range)) break;
    count++;
    zzlNext(zl,&eptr,&sptr);
}
```

ziplist 是扁平连续内存，没有 rank/span，无法做两个 rank 相减，只能线扫。

### zlexcountCommand 完全对称

`zlexcountCommand` 用同样的算法，只是把 First/LastInRange 换成 LexRange 版本，按 member 字典序计数。
