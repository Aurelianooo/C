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
