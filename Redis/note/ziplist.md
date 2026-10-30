# ziplist 模块问答

> 对应源码:`src/ziplist.h`、`src/ziplist.c`
> 相关文件:`src/endianconv.h`、`src/t_list.c`、`src/t_zset.c`

---

## 一、ziplist 是什么?和 adlist 有什么区别?

一句话:**一个为了极致省内存而设计的、连续内存存储的"伪双端链表"**。它同时能存字符串和整数,整数是真按二进制存(不是存字符串形式)。

和 `adlist`(真正的双端链表)的根本区别:

| | adlist | ziplist |
|---|---|---|
| 内存布局 | 每个节点单独 `malloc`,靠指针相连 | 一整块连续内存,节点挤在一起 |
| 节点大小 | 固定(`listNode` + `void*`) | 可变(几字节到几 KB) |
| 省内存 | 差(每个节点都有 prev/next/value 三个指针) | 好(没有指针,只有长度字段) |
| 是否允许重复 | 允许 | ziplist 本身不去重,唯一性由上层保证 |

缺点:任何增删都要对整个 ziplist 做 `realloc` + `memmove`,只适合**小规模、元素短**的场景。Redis 用它作为 hash / zset / list 类型在"元素很少"时的底层编码。

### 整体内存布局

```
<zlbytes><zltail><zllen><entry>...<entry><zlend>
  4字节    4字节   2字节   ...              1字节(固定255)
```

- **zlbytes**(4B):整个 ziplist 占多少字节,方便 `realloc` 时不用遍历就能知道大小
- **zltail**(4B):最后那个 entry 距离起始位置的偏移量,从尾部 pop 不用从头遍历
- **zllen**(2B):entry 数量,超过 `UINT16_MAX-1`(65534)时作废,必须遍历才算得出真实数量
- **zlend**(1B):固定 `255`,标记末尾

---

## 二、zltail 和 ZIPLIST_ENTRY_TAIL 是什么关系?既然有指针宏,为什么还要存 zltail?

**`ZIPLIST_ENTRY_TAIL` 不是真正的"指针",它本身就靠 `zltail` 算出来**:

```c
#define ZIPLIST_ENTRY_TAIL(zl)  ((zl)+intrev32ifbe(ZIPLIST_TAIL_OFFSET(zl)))
```

做的事就是:`zl 起始地址 + zltail 偏移量 = 尾节点地址`。所以 `zltail` 是**数据**,`ZIPLIST_ENTRY_TAIL` 是**用这个数据算出地址的宏**,两者是上下游关系,不是二选一。

### 为什么必须存"偏移量"而不是存"指针地址"本身?

因为 ziplist 是一整块 `realloc` 出来的内存,每次插入/删除都要 `ziplistResize`,`realloc` 可能把整块数据搬到别处 —— `zl` 起始地址会变。

- 存**绝对指针**:`realloc` 搬走后变成野指针。
- 存**偏移量**:不管 `zl` 被搬到哪,`新 zl + 偏移量` 永远能重新定位。

这和 `__ziplistInsert` / `__ziplistDelete` 里到处 `offset = p - zl; ... p = zl + offset;` 是同一个道理。

### zltail 也不是"存了就不用维护"

每次头/尾插入、删除时,`zltail` 都要重新计算并写回。本质是用"每次修改时多维护一个 4 字节字段"的代价,换取"查尾 O(1)"的好处。

---

## 三、每个 entry 里的元素可以是不同类型的吗?

可以,这正是 `encoding` 字段存在的意义 —— **每个 entry 独立决定自己存字符串还是整数,entry 之间互不影响**。

例如一个 ziplist 可以:

```
entry1: "hello"   -> 字符串编码
entry2: 100       -> 整数编码(立即数,直接藏在 encoding 里)
entry3: 70000     -> int32 整数编码
entry4: "world"   -> 字符串编码
```

### 关键区分:不是"存储不同类型",而是"每个 entry 有自己的编码"

- ziplist 本身没有类型,只是一段字节流(本质 `unsigned char*`)。
- 类型信息是每个 entry 自己带的,藏在 entry 头部的 `encoding` 字段里。
- 读取时 `zipEntry` / `ZIP_DECODE_LENGTH` 先解析出该 entry 的 `encoding`,才知道怎么读后面数据。

这与 `intset` 相反:`intset` 是**整个集合统一一种编码**,ziplist 是**每个元素各自为政**。

### 为什么这么设计

ziplist 是 hash / zset / list 在"数据量小"时的紧凑编码,这些类型允许混合存放字符串和数字。Redis 还会做优化:`__ziplistInsert` 里的 `zipTryEncoding` 尝试把推入的字符串解析成整数,成功就用更紧凑的整数编码,否则按字符串存。所以同一个 ziplist 里混着整数和字符串是常态。

---

## 四、zlentry 结构体的 prevrawlensize 和 prevrawlen 是什么关系?

两个成员描述**同一个东西(前一个节点的长度),但从两个角度看**:

| 成员 | 含义 | 单位 |
|------|------|------|
| `prevrawlen` | 前一个节点的**实际长度值** | 字节数(一个数) |
| `prevrawlensize` | 存这个长度值**需要占用几个字节** | 字节数(编码开销) |

### 例子:前一个节点长 300 字节

- `prevrawlen = 300`(前节点真实长度 300)
- `prevrawlensize = 5`(因为 300 >= 254,得用 5 字节才存得下)

前一个节点长 100 字节时:`prevrawlen = 100`,`prevrawlensize = 1`。

### 为什么同时需要这两个

ziplist 里 entry 是**变长、紧密排列的字节流**,指针每走一步都先要搞清楚"这一步跨多少字节":

1. `prevrawlensize`:prevlen 字段占几个字节 → 决定"当前 entry 数据从哪开始"
2. `prevrawlen`:前一个节点真实长度 → 决定"往前跳多少字节能到前一个节点"

所以 `zipEntry` 里:

```c
ZIP_DECODE_PREVLEN(p, e.prevrawlensize, e.prevrawlen);               // 解析 prevlen 字段
ZIP_DECODE_LENGTH(p + e.prevrawlensize, e.encoding, e.lensize, e.len); // 跳过 prevlen 再解析 encoding
e.headersize = e.prevrawlensize + e.lensize;                          // 头部总长
```

`p + e.prevrawlensize` 是因为 encoding 字段紧跟在 prevlen 字段之后,得先用 `prevrawlensize` 跳过 prevlen 那一段。

**总结**:`prevrawlen` 是内容(前节点多长),`prevrawlensize` 是这个内容的存储开销(存这个长度要几个字节)。前者用于向后回跳遍历(`ziplistPrev` 里 `return p-entry.prevrawlen;`),后者用于定位 entry 内部边界和计算节点总长。

---

## 五、为什么 `__ziplistInsert` 名字前有两个下划线?

`__` 前缀是 C 里约定俗成的"**内部/私有函数,别直接调用**"标记,不是编译器强制的 `private`,纯粹是给人看的信号。

这仨函数的对应关系:

| 内部 `__` 函数 | 公开替身 |
|--------------|---------|
| `__ziplistInsert` | `ziplistInsert` |
| `__ziplistDelete` | `ziplistDelete`、`ziplistDeleteRange` |
| `__ziplistCascadeUpdate` | 无公开替身,只被内部调用 |

为什么要分:底层 `__` 函数会 `realloc` 改变 `zl` 地址,公开函数需额外用偏移量保存/恢复指针。如 `ziplistDelete`:

```c
unsigned char *ziplistDelete(unsigned char *zl, unsigned char **p) {
    size_t offset = *p - zl;          // 记下 p 的相对偏移
    zl = __ziplistDelete(zl, *p, 1);  // 可能 realloc,zl 地址变了
    *p = zl + offset;                 // 靠偏移量还原 *p
    return zl;
}
```

`p` 是 `unsigned char **`(二级指针),就是要原地修改调用方指针,因为 `realloc` 后 `*p` 会失效。

---

## 六、`__ziplistInsert` 这个函数在干嘛?

签名:`__ziplistInsert(zl, p, s, slen)` —— 把长度 `slen` 的字节串 `s` 插入到 `p` 所指位置**之前**。分 6 步:

### 第 1 步:算出新节点的 prevlen(它前面是谁)

```c
if (p[0] != ZIP_END) {
    entry = zipEntry(p);
    prevlen = entry.prevrawlen;   // p 指向节点的前驱 = 新节点的前驱
} else {
    unsigned char *ptail = ZIPLIST_ENTRY_TAIL(zl);
    if (ptail[0] != ZIP_END)
        prevlen = zipRawEntryLength(ptail);  // 插到尾部:表尾节点变成前驱
}
```

### 第 2 步:决定用整数还是字符串编码

```c
if (zipTryEncoding(s,slen,&value,&encoding))
    reqlen = zipIntSize(encoding);   // 能转整数 -> 按整数宽度算
else
    reqlen = slen;                    // 不能 -> 按字符串原长算
```

### 第 3 步:算新节点完整长度 reqlen

```c
reqlen += zipPrevEncodeLength(NULL, prevlen);   // + prevlen 字段长度
reqlen += zipEncodeLength(NULL, encoding, slen); // + encoding 字段长度
```

entry = prevlen 字段 + encoding 字段 + 值。传 `NULL` 让函数只返回"编码需要几字节"。

### 第 4 步:检查后置节点要不要跟着扩展(nextdiff)

```c
nextdiff = (p[0] != ZIP_END) ? zipPrevLenByteDiff(p, reqlen) : 0;
```

`nextdiff` = 编码新长度所需字节数 − 后置节点 prevlen 已有字节数。0 表示不用动,4 表示后置节点 prevlen 要从 1 字节扩成 5 字节。

### 第 5 步:realloc 扩容 + memmove 腾位置

```c
offset = p - zl;                         // 先记偏移(要 realloc 了!)
zl = ziplistResize(zl, curlen+reqlen+nextdiff);
p = zl + offset;                         // 靠偏移量还原 p
```

若插在中间,还要 memmove 后置节点整体后移,并更新 `zltail`。

### 第 6 步:写入新节点 + 级联更新

```c
if (nextdiff != 0)
    zl = __ziplistCascadeUpdate(zl, p+reqlen);  // 后置节点变长了,向后连锁修正

p += zipPrevEncodeLength(p, prevlen);   // 写 prevlen
p += zipEncodeLength(p, encoding, slen); // 写 encoding
// 写值(字符串 memcpy / 整数 zipSaveInteger)
ZIPLIST_INCR_LENGTH(zl, 1);             // zllen + 1
```

### 为什么复杂度是 O(N²)

插入本身要 memmove 后面所有节点(平均 N/2 个),已是 O(N);若触发级联更新,还要向后连锁扫描,叠加后最坏 O(N²)。

**一句话**:定前驱 → 选编码 → 算总长 → 看要不要撑大后置节点 → realloc + memmove 挪位置 → 写入新节点 → 必要时级联修正,全程用相对偏移量应对 realloc 的地址搬移。

---

## 七、ziplist 是"唯一元素"吗?ziplistCompare 在哪被调用?

### ziplist 本身不去重

`__ziplistInsert` 就是"在 p 前塞一个字节串 s",**不管元素是否已存在**,塞两个一样的 `"foo"` 就是两个一样的 entry。

"元素唯一"是**上层类型的语义,不是 ziplist 的职责**:

- Hash 的 field、ZSet 的 member 天然唯一 → 靠上层 `hashTypeSet` / `zsetAdd` 先生成/去重。
- List 则允许重复 → `LPUSH` 两次 `"foo"` 就是两个 `"foo"`。

### ziplistCompare 的调用位置

`ziplistCompare` 在 `ziplist.c` 内部**不被调用**,它是对外公开的 API,被以下上层代码调用:

| 位置 | 语境 |
|------|------|
| `t_list.c:393` | List 类型查找某个元素 |
| `t_zset.c:1350` | ZSet 查找 member |
| `t_zset.c:3742` | ZSet 另一个查找场景 |

典型用法(t_zset.c:1350 附近),上层自己控制遍历,逐个 entry 调 `ziplistCompare`:

```c
eptr = ziplistIndex(zl, 0);
while (eptr != NULL) {
    if (ziplistCompare(eptr, ele->ptr, sdslen(ele->ptr)))
        break;              // 找到了
    eptr = ziplistNext(zl, eptr);
}
```

### 它和 ziplistFind 的分工

- `ziplistFind`:遍历整个 ziplist,找到第一个相等 entry,返回指针。
- `ziplistCompare`:只比较**某个指定 entry**(由 `p` 定位)和传入值是否相等,返回 1/0。

上层类型往往自己控制遍历逻辑,只需要"比较单个 entry"这个原子能力即可,不必用 `ziplistFind` 那种包办遍历的重型接口。

### 两个小细节

1. `ziplistCompare` 的整数比较**不看编码方式**——entry 里存 `100` 是 int16、传入的 `100` 是字符串,一样判等。它会把两边数值都解出来再比(不关心编码,因为不同实现可能用不同编码表示同一整数)。
2. 它声明在 `ziplist.h`,是正式公开 API。
