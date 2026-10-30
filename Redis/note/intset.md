# intset 模块问答

> 对应源码：`src/intset.h`、`src/intset.c`
> 相关文件：`src/endianconv.h`

---

## 一、为什么 intset 结构体里要有个 `encoding` 字段？

```c
typedef struct intset {
    uint32_t encoding;   // 编码方式
    uint32_t length;     // 元素数量
    int8_t contents[];   // 元素数组(柔性数组)
} intset;
```

`encoding` 记录当前 `contents` 里所有元素用的是哪种整数宽度，它是 intset 实现**自适应宽度**的核心。

intset 只存整数，但整数的取值范围差别很大。为了省内存，Redis 不统一用 8 字节的 `int64_t` 去存所有元素，而是根据"实际需要的最大宽度"来选编码：

| 编码宏 | 值 | 占字节数 | 能表示的范围 |
|--------|-----|---------|-------------|
| `INTSET_ENC_INT16` | `sizeof(int16_t)` = 2 | 2 字节 | -32768 ~ 32767 |
| `INTSET_ENC_INT32` | `sizeof(int32_t)` = 4 | 4 字节 | -2^31 ~ 2^31-1 |
| `INTSET_ENC_INT64` | `sizeof(int64_t)` = 8 | 8 字节 | -2^63 ~ 2^63-1 |

三个编码宏的值直接取 `sizeof(...)`，所以 `encoding` 字段里存的数字正好就是每个元素的字节宽度，读取时可以直接用来做指针偏移。

`encoding` 保证了：**所有元素等宽、连续排布**，因此可以用"基址 + 下标 × 宽度"的方式 O(1) 定位任意一个元素，也是二分查找 `intsetSearch` 能成立的前提。

---

## 二、为什么 `intsetNew` 里用 `intrev32ifbe(INTSET_ENC_INT16)`"4 字节转换函数"去处理一个 2 字节的值，不会出问题吗？

```c
is->encoding = intrev32ifbe(INTSET_ENC_INT16);
```

两个原因，缺一不可。

### 1. `intrev32ifbe` 在小端机器上就是恒等函数

看 `endianconv.h`：

```c
#if (BYTE_ORDER == LITTLE_ENDIAN)
#define intrev16ifbe(v) (v)
#define intrev32ifbe(v) (v)
#define intrev64ifbe(v) (v)
#else
#define intrev16ifbe(v) intrev16(v)
#define intrev32ifbe(v) intrev32(v)
#define intrev64ifbe(v) intrev64(v)
#endif
```

在 x86 等小端机器上，`ifbe`（from byte endian）系列宏**直接原样返回，不做任何转换**。所以 `intrev32ifbe(INTSET_ENC_INT16)` 就等于 `INTSET_ENC_INT16` 本身，值还是 `2`，不存在"拿 4 字节函数去反转 2 字节数据"的问题。

### 2. 字段本身是 `uint32_t`，所以必须用 32 位版本

`intset.encoding` 这个字段的类型是 `uint32_t`（4 字节），而不是 2 字节。若真在大端机器上运行，这个 4 字节字段的内存字节序需要按 32 位来反转，所以必须用 `intrev32ifbe`，而不是 `intrev16ifbe`。

这里"2 字节"是**编码值的大小**（表达"每个元素 2 字节宽"这个含义），而 `intrev32ifbe` 处理的是**`encoding` 字段这个 4 字节容器**的字节序。两者作用对象不同，并不矛盾。

---

## 三、`intsetUpgradeAndAdd` 里为什么 `prepend = value < 0 ? 1 : 0;` 要和 0 比较？这看起来不像在比较元素大小啊？

```c
int prepend = value < 0 ? 1 : 0;
```

### 先把三元运算符读对

`prepend = value < 0 ? 1 : 0;` 的真实含义是：

- `value < 0` 为**真**（value 是负数）→ `prepend = 1`
- `value < 0` 为**假**（value 是 0 或正数）→ `prepend = 0`

所以 `prepend` 的语义就是"新值是不是负数"：负数 → 1，非负 → 0。

### 为什么判断符号就等价于判断"最小还是最大"

`intsetUpgradeAndAdd` 只在**编码升级**时才被调用。编码升级意味着新 value 用当前编码（如 INT16）表示不了，必须换更大的编码。intset 是**有序**的，那么一个"超出当前表示范围"的新值只有两种可能：

1. 比所有现有元素都**小** —— 小于 INT16_MIN(-32768)，必然是**负数**；
2. 比所有现有元素都**大** —— 大于 INT16_MAX(32767)，必然是**非负**。

因此根本不需要真的拿 value 去和现有元素逐个比，**判断符号就等价于判断"新值是最小还是最大"**：

- `value < 0` → 新值是最小的 → 插到最前面 → `prepend = 1`
- `value >= 0` → 新值是最大的 → 插到最后面 → `prepend = 0`

这正是注释里"新元素比原来所有元素都大，也即 `prepend == 0`"的由来：比所有元素都大也就是最大值，所以 prepend = 0（追加到尾部）。

### 结合后续代码看 prepend 的一鱼两吃

```c
is = intsetResize(is, intrev32ifbe(is->length)+1);

// 原有元素整体后移：prepend==1 时统一右移 1 位，留出头部空位
while(length--)
    _intsetSet(is, length+prepend, _intsetGetEncoded(is, length, curenc));

if (prepend)
    _intsetSet(is, 0, value);                       // 新值最小 -> 放 0 号位
else
    _intsetSet(is, intrev32ifbe(is->length), value); // 新值最大 -> 放尾部
```

`prepend` 这个 0/1 值同时承担两个职责：

1. 决定旧元素后移几格（`length+prepend`：prepend==1 时旧下标整体 +1，prepend==0 时不变）；
2. 决定新值落点（头部 0 号位 or 尾部 length 号位）。

**总结**：跟 0 比较不是"比较元素大小"的字面写法，但它等价于"判断新值落在所有元素之前还是之后"——因为升级场景下超出范围的值，要么是负的（新最小），要么是非负的（新最大）。

---

## 四、intset 的本质是什么？

intset 就是一段连续内存里存的、按值**从小到大严格有序、不重复**的整数数组；叠加 `encoding` 字段后，它更准确地说是一个**所有元素等宽的自适应有序整数数组**。

| 属性 | 说明 |
|------|------|
| 有序 | `intsetSearch` 用二分查找，依赖有序性 |
| 不重复 | `intsetAdd` 插入前先 `intsetSearch`，找到则直接返回 |
| 连续数组 | `contents[]` 是一整块内存，元素挤在一起 |
| 同宽编码 | 所有元素用同一种宽度，由 `encoding` 决定 |

和普通数组的两点区别：

1. **长度动态** —— `intsetResize` 用 `zrealloc` 扩容/缩容；
2. **宽度也动态** —— 出现超范围大数时，`intsetUpgradeAndAdd` 把整个数组升级到更大编码。

### 一个补充：只能升级、不能降级

- 先存 100、200（INT16）；
- 再存 70000（INT32），整个数组从 16 位升级到 32 位；
- 之后即使把 70000 删掉，数组也**不会**降回 16 位。

编码宽度只随"曾经出现过的最大值"单向变大。因为实际场景里集合要么一直是小整数，要么一开始就会有大数，很少出现"先有大数后又全变小数"的极端情况，不值得为降级再写一套反向搬迁逻辑。

更精确的表述：**intset 是一个"所有元素等宽、严格递增、无重复"，且宽度只能单向升级的整数数组。** 这也是它只适合存"纯小规模整数集合"、一旦集合变大或出现字符串就交给另一种结构的原因。

---

## 五、intset 升级后换的"另一种结构"（hashtable 编码的 set）是什么？在哪实现？

Set 类型（`SET` 命令对应的集合）在 Redis 3.0 有两种底层编码：

| 编码值 | 底层数据结构 | 适用情况 |
|--------|-------------|---------|
| `OBJ_ENCODING_INTSET` | `intset` | 元素全为整数，且数量不超过 `set-max-intset-entries`（默认 512） |
| `OBJ_ENCODING_HT` | `dict`（哈希表） | 出现字符串，或数量超阈值，或无法用 intset 表示 |

"hashtable 编码的 set" 指的就是第二种 `OBJ_ENCODING_HT`，底层就是已读过的 `dict` 字典：哈希表的 key 是集合元素，value 全为 `NULL`，靠"键是否存在"表达成员关系（字典天然保证键不重复）。

### 实现位置

- **Set 命令实现**：`src/t_set.c`
- **编码转换决策**：`src/t_set.c` 的 `setTypeAdd`、`setTypeConvert`；**创建默认编码**：`src/object.c` 的 `createSetObject`

关键点：

```c
// object.c —— 创建时默认就是 hashtable 编码
robj *createSetObject(void) {
    dict *d = dictCreate(&setDictType, NULL);
    robj *o = createObject(OBJ_SET, d);
    o->encoding = OBJ_ENCODING_HT;
    return o;
}
```

```c
// t_set.c —— 添加元素时决定是否转 intset / 升级
int setTypeAdd(robj *subject, robj *value) {
    if (subject->encoding == OBJ_ENCODING_HT) {
        // 已是 hashtable，直接 dictAdd
    } else if (subject->encoding == OBJ_ENCODING_INTSET) {
        if (isObjectRepresentableAsLongLong(value, NULL) == C_OK) {
            // 整数 -> 继续用 intset
        } else {
            // 字符串 -> 触发升级 intset -> hashtable
            setTypeConvert(subject, OBJ_ENCODING_HT);
        }
    }
}
```

```c
// t_set.c —— intset -> dict 的转换：逐个取出 intset 元素塞进新字典
void setTypeConvert(robj *setobj, int enc) {
    if (enc == OBJ_ENCODING_HT) {
        dict *d = dictCreate(&setDictType, NULL);
        // 遍历 intset，把每个整数包装成字符串对象后 dictAdd
        setobj->encoding = OBJ_ENCODING_HT;
        setobj->ptr = d;
    }
}
```

### 对应关系小结

```
SET 类型 (robj, type = OBJ_SET)
 ├── encoding = OBJ_ENCODING_INTSET  ->  ptr 指向 intset
 └── encoding = OBJ_ENCODING_HT      ->  ptr 指向 dict(哈希表)
```

intset 只适合小规模纯整数集合；一旦集合变大、或混入字符串、或元素无法表示为整数，就由 `t_set.c` 的 `setTypeConvert` 把有序定宽数组整体换成 `dict` 哈希表。
