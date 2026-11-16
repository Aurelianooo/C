# hyperloglog 模块源码阅读笔记

> Redis 3.0 的基数统计实现。用约 12KB 固定内存，估算任意大集合的基数（不同元素个数），标准误差约 0.81%。

---

## 一、为什么叫 HyperLogLog？

拆成三部分，对应算法演进史：

1. **Log**：算法基于"对数"直觉——记录哈希值的"前导零个数"，这是元素数的对数量级。
2. **LogLog**：早期版本（Flajolet–Martin → LogLog），用极小存储（log log n 量级）做基数统计。
3. **Hyper**：2007 年 Flajolet 等人的改进版——分 m 组寄存器、取调和平均，把误差压到 `1.04/√m`。即"超越/升级版 LogLog"。

解决的问题：**基数统计**（集合里不同元素个数，如"今天多少个不同 IP 访问"）。传统去重要 O(n) 内存，HyperLogLog 用 12KB 固定内存即可。

---

## 二、`struct hllhdr` 逐字段

```c
struct hllhdr {
    char magic[4];      /* "HYLL" */
    uint8_t encoding;   /* HLL_DENSE 或 HLL_SPARSE */
    uint8_t notused[3]; /* 保留，必须为 0 */
    uint8_t card[8];    /* 缓存的基数，小端序 */
    uint8_t registers[];/* 数据区，柔性数组 */
};
```

| 字段 | 含义 |
|------|------|
| `magic[4]` | 魔数 "HYLL"，标识合法 HLL 对象 |
| `encoding` | 编码方式：DENSE(0)/SPARSE(1)/RAW(255 内部) |
| `notused[3]` | 3 字节填充，预留未来，必须 0 |
| `card[8]` | 缓存的上次基数结果，最高位复用为"缓存有效"标志 |
| `registers[]` | 柔性数组，存 16384 个寄存器的数据 |

### card 缓存标志位技巧

```c
#define HLL_INVALIDATE_CACHE(hdr) (hdr)->card[0] |= (1<<7)   // 最高位置1=失效
#define HLL_VALID_CACHE(hdr) (((hdr)->card[0] & (1<<7)) == 0)
```

`card[0]` 最高位借用为"缓存是否有效"标志：数据变了置 1（失效），PFCOUNT 时最高位为 0 才直接用缓存。

### 关键宏

| 宏 | 值 | 含义 |
|----|----|----|
| `HLL_P` | 14 | 精度参数，越大误差越小 |
| `HLL_REGISTERS` | 16384 | 寄存器数 = 2^14 |
| `HLL_BITS` | 6 | 每寄存器位数（能记 0~63 前导零） |
| `HLL_HDR_SIZE` | 16 | 头大小（4+1+3+8） |
| `HLL_DENSE_SIZE` | 12304 | DENSE 编码总大小（16 + 12288） |

---

## 三、核心算法思想

### 直觉：用"最稀有事件"反推"总数"

- 一件概率为 p 的事，平均做 1/p 次成功一次；
- 反过来：观察到"最稀有一次"的稀有度，反推总共试了多少次。

哈希一个元素 ≈ 抛一串公平硬币；"前导零个数 ρ" = "连续几个 0 才遇到 1"；前导零 ≥ k 的概率 = 1/2^k。

**结论**：记"见过的最大的前导零 maxρ"，基数 ≈ 2^maxρ。因为看到"前导零=k"这么稀有（概率 1/2^k），意味着大约见过 2^k 个元素。

### 为什么 log

前导零个数是元素基数的对数量级，所以存储量是 log log n。

### HyperLogLog 的改进：分桶 + 调和平均

单计数器方差太大（某个元素碰巧前导零爆长会严重高估）。Redis 分 m=16384 桶：

```
hash(64位) = MurmurHash64A(element)
              ├─ 低14位(index = hash & 0x3FFF) → 选桶
              └─ 高50位 → 数前导零 ρ
registers[index] = max(registers[index], ρ)
```

每个桶只记最大 ρ。最后对 m 个桶的 2^ρ 做**调和平均**（抑制离群大值），乘修正系数，误差降到 `1.04/√16384 ≈ 0.81%`。

---

## 四、`hllPatLen`：为什么从低位往高位数前导零

```c
hash = MurmurHash64A(ele, elesize, 0xadc83b19ULL);
index = hash & HLL_P_MASK;      // 低14位 → 选桶
hash |= ((uint64_t)1<<63);      // 强制最高位=1，保证循环终止
bit = HLL_REGISTERS;            // 1<<14，第一个不用于选桶的位
count = 1;                      // 把结束序列的"1"也算进去
while((hash & bit) == 0) {
    count++;
    bit <<= 1;                  // 往高位（往左）左移
}
```

### 关键理解

- **方向没错**：`index` 取低 14 位（bit 0~13），`bit` 从 `1<<14` 往高位左移数前导零，这正是"从低往高"数。你担心的"从右往左"是误解——它就是从 bit14 往 bit63 走。
- **count 不清零是对的**：count 初始 1（把"终结的那个 1"算进去），遇到 1 就退出循环，保留 count。
- **`hash |= 1<<63`**：强制最高位为 1，防止极端情况（高 50 位全是 0）死循环。

### 为什么不从高位往低位数

数学上两者等价（均匀哈希，正反看分布一样），但实现上没得选：选桶用 `& HLL_P_MASK` 天然取**低位**，剩下高位顺带用左移数零，最自然省事。

---

## 五、DENSE vs SPARSE 编码

### 区别

| | DENSE | SPARSE |
|---|-------|--------|
| 存储 | 16384×6bit 固定打包成 12288 字节 | 游程编码（ZERO/XZERO/VAL）只存非零段 |
| 更新 | O(1) 位运算 | O(N) 定位 + 拆分/合并 |
| 内存 | 基数小也固定 12288 | 基数小时极省 |
| 适用 | 基数大 | 基数小 |

### 为什么分两种：内存 vs 更新速度的权衡

- 基数小（~2000-3000 以下）：16384 个寄存器大多还是 0，稀疏编码省内存；
- 基数大：稀疏更新越来越慢、内存优势消失，转 DENSE 换 O(1)。

阈值 `server.hll_sparse_max_bytes` 决定何时 SPARSE→DENSE。

### 稀疏编码三种操作码

| 操作码 | 首字节特征 | 含义 |
|--------|-----------|------|
| ZERO | `00xxxxxx` | 连续 `(低6位+1)` 个 0（最长 64） |
| XZERO | `01xxxxxx` | 连续 `((低6位<<8|下字节)+1)` 个 0（最长 16384，2 字节） |
| VAL | `1vvvvvxx` | 值 `(vvvvv+1)` 的寄存器，连续 `(xx+1)` 个 |

---

## 六、`hllSparseAdd` 做了什么

1. `hllPatLen` 算出新值 `count` 和寄存器 `index`；
2. **遍历游程**，累加 `first` 定位到覆盖 `index` 的 opcode（最热循环）；
3. **分情况**：A 现有值 ≥ count 不用改；B VAL 单值直接改；C 单 ZERO 直接改；D 通用·**把游程原地拆成"左段 + 提值 + 右段"**；
4. `memmove`/`memcpy` 原地替换（前面已 `sdsMakeRoomFor` 预留 3 字节）；
5. **合并相邻同值 VAL** 保持紧凑 → `HLL_INVALIDATE_CACHE` 失效缓存。

`if (count > HLL_SPARSE_VAL_MAX_VALUE) goto promote` 或更新后超过 `server.hll_sparse_max_bytes` 就转 DENSE。

### `sdslen(o->ptr)` 的澄清

`o->ptr` 本身是 sds（指向 HLL 数据），`sdslen(o->ptr)` 是 O(1) 读取 sds 头里**已经存好的 len 字段**，得到 HLL 对象的字节总长，不是现算大小。

判断 `sdslen(o->ptr) + deltalen > server.hll_sparse_max_bytes` 意思是"当前长度 + 增量 = 更新后长度"，超过阈值就转稠密。

---

## 七、registers 存的不是 hash 值本身

精确：registers 存的是**每个桶见过的"最大前导零个数"**（0~63 的小整数），不是 hash 值本身。

```
element → MurmurHash64A → hash
                            ├─ 低14位 → 选桶 index
                            └─ 高50位 → 前导零 count
registers[index] = max(registers[index], count)   // 存最大前导零，不是 hash
```

hash 值是中间产物，用完即弃。每个寄存器只需 6 bit（存 0~63），而非 64 位 hash。

---

## 八、`hllCount`：调和平均反推基数

### 核心公式

```
E = α_m · m² · 1 / (Σ 2^(-ρj))
```

- `m` = 16384；
- `ρj` = 第 j 个寄存器的前导零个数；
- `α_m` = 0.7213/(1+1.079/m)，修正系数。

代码对应：

```c
double m = HLL_REGISTERS;
double alpha = 0.7213/(1+1.079/m);
E = hllDenseSum(...);             // E = Σ 2^(-ρj)
E = (1/E) * alpha * m * m;        // 核心公式
```

### 为什么是"2 的负 ρj 次方"求和

`Σ 2^(-ρj)` 求和再取倒数 = 对所有桶的 `2^ρj` 做**调和平均**。用调和平均而非算术平均，是为了抑制个别桶的前导零爆长（离群大值）拉爆平均值——这正是 LogLog 用算术平均失败的教训。

### 打表优化：PE[64]

`2^(-ρj)` 只有 64 种取值，预计算 `PE[j] = 2^(-j)` 存表，求和时查表，省 16384 次 pow 调用。

### 两端校正

1. **小基数 LinearCounting**（`E < 2.5m 且 ez≠0`）：`E = m·ln(m/ez)`，用"还有多少寄存器为 0（ez）"估计基数，小基数下比调和平均准。
2. **中等基数偏差校正**（16384 桶 且 E<72000）：多项式回归 `bias = ...`（魔法数字是实验拟合的系数），从估计里减去系统偏差。

---

## 九、总结

HyperLogLog 完整链路：

1. `hllAdd`：hash 元素 → 低 14 位选桶、高 50 位数前导零 → 更新该桶最大前导零；
2. `hllCount`：遍历 16384 寄存器，用调和平均公式 `E = α·m²·(1/Σ2^-ρ)` 反推基数，再按小/中基数做校正。

关键设计：极小内存（12KB）、固定误差（~0.81%）、寄存器只存"前导零个数"、稀疏/稠密两段编码按基数切换。

---

# 附：hyperloglog.c 命令层问答（第三阶段 · PF 开头的函数）

> 以下是对 hyperloglog.c 中 `pf` 开头命令及配套实现（hllAdd/hllCount/合并等）的讲解，侧重"命令层的运作与设计动机"。

---

## 十、HLL 算法核心：为什么用 6-bit 寄存器估基数

### 直觉：一个"最长 0 串"猜元素个数

哈希元素得到一个均匀 64 位整数，"末尾 0 串长度"越长的元素越稀有（概率 1/2^k）。观察到"最长 0 串 = k"，说明大约见过 2^k 个元素。

### hllPatLen：哈希 → 拆成"选桶索引 + 0 串长度"

```c
int hllPatLen(unsigned char *ele, size_t elesize, long *regp) {
    hash = MurmurHash64A(ele,elesize,0xadc83b19ULL);
    index = hash & HLL_P_MASK;      // 低 14 位 → 选桶
    hash |= ((uint64_t)1<<63);      // 最高位置1，保证循环终止
    bit = HLL_REGISTERS;            // 从第 14 位开始数
    count = 1;
    while((hash & bit) == 0) { count++; bit <<= 1; }
    *regp = (int) index;
    return count;
}
```

把 64 位哈希拆两部分：低 14 位选寄存器（分桶），高位（14~63）数连续 0 直到遇 1。

### hllDenseAdd：寄存器只存"见过的最长 0 串"（取 max）

```c
count = hllPatLen(ele,elesize,&index);
HLL_DENSE_GET_REGISTER(oldcount,registers,index);
if (count > oldcount) {
    HLL_DENSE_SET_REGISTER(registers,index,count);
    return 1;    // 寄存器变了 → 基数可能变
} else {
    return 0;    // 没变
}
```

`return 1/0` 让命令层 `pfaddCommand` 判断"这次 add 是否让 set 变大"，决定回 1 还是 0。

---

## 十一、一次 add 只碰一个桶 + 为什么分桶

### 一个元素只更新一个寄存器

`index = hash & HLL_P_MASK` 是确定的一个值，所以一次 PFADD 只读写字 index 那个寄存器，其余 16383 桶不动。

- **PFADD**：O(1)，只碰落到的那 1 桶；
- **PFCOUNT**：遍历全部 16384 桶，调和平均成总估算。

时间线：`PFADD a → hash→index=777→只更新第777桶`；`PFCOUNT → 遍历所有桶算总数`。

### 为什么分 16384 桶而不是一个大桶

单计数器记"最长 0 串"方差巨大（一个元素碰巧 20 个 0 就估成 2^20），误差 100% 量级不可用。

分桶后，对 16384 个独立桶的估计做**调和平均**，方差被摊薄 √16384 ≈ 128 倍，误差降到 `1.04/√16384 ≈ 0.81%`。个别桶的坏样本（超长 0 串）只污染 1/16384，被其余桶稀释。

`HLL_P = 14` 即桶数 `2^14 = 16384`，是"内存(12KB) vs 精度(0.81%)"的甜点。

---

## 十二、元素不多时，HLL 反而吃亏

| 场景 | 精确方案 | HLL |
|------|---------|-----|
| 基数小 | set + SCARD 精确、快、省 | 有误差、不一定省、算得多 |

HLL 的价值是"用 0.8% 误差换恒定 12KB 内存"，只在该交换成立时才有意义：**基数巨大、能容忍误差**。元素少时内存本来没涨起来，这个交换就亏了。

`createHLLObject` 一开始就用 sparse（游程压缩），小基数时几百字节而非铺满 12KB，正是"基数小也别浪费"的体现。

---

## 十三、DENSE 编码的位运算：HLL_DENSE_GET/SET_REGISTER

dense 把 16384 个 6-bit 寄存器紧密塞进 12288 字节，寄存器跨越字节边界，需位运算访问。

### 定位

```c
_byte = regnum*HLL_BITS/8;   // 起始字节号
_fb   = regnum*HLL_BITS&7;   // 字节内 bit 偏移
```

### 读（GET）

```c
target = ((b0 >> _fb) | (b1 << _fb8)) & HLL_REGISTER_MAX;
```

寄存器跨两字节：b0 右移取右半截，b1 左移补左半截，`&63` 清高位。

### 写（SET）：分两步（清位 + 置位），两字节分别处理

```c
_p[_byte]   &= ~(63 << _fb);   _p[_byte]   |= _v << _fb;   // 低段
_p[_byte+1] &= ~(63 >> _fb8);  _p[_byte+1] |= _v >> _fb8;  // 高段
```

### 越界读 b1 安全：sds 隐式 \0

读最后一个寄存器会 `_byte+1` 越界，但 HLL 底层是 sds，sds 总带隐式 `\0`，那 1 字节合法存在，且 `&63` 会清掉污染位，省去分支。

### 用宏 + 手动循环展开

宏内联避免函数调用开销；`hllDenseSum` 对 `16384/6bit` 的特殊路径手写 16 寄存器/迭代的循环展开 + `PE[]` 查表，保证 PFCOUNT 最热路径最快。

---

## 十四、SPARSE 编码：三种 opcode 的游程压缩

小基数时 16384 桶大多为 0，用游程压缩只记非零段。

| opcode | 字节 | 含义 |
|--------|------|------|
| ZERO | `00xxxxxx` | 1~64 个连续 0 |
| XZERO | `01xxxxxx yyyyyyyy` | 1~16384 个连续 0（14bit） |
| VAL | `1vvvvvxx` | 1~4 个连续寄存器，值 1~32 |

高位区分：ZERO=`00`、XZERO=`01`、VAL=`1`。所有参数都 `+1` 存（0 无意义，拿来表达 1）。

### 只有 3 个非零寄存器的例子

```
XZERO:1000  VAL:2,1  ZERO:19  VAL:3,2  XZERO:15362
```

共 7 字节，比 dense 的 12288 字节省 1800 倍。

### hllSparseAdd 的分裂：把 0 游程劈成三段

更新中间某个 0 → `左ZERO + 新VAL + 右ZERO`。最坏 XZERO 拆成 `XZERO-VAL-XZERO`（2→5 字节），所以先 `sdsMakeRoomFor(o->ptr,3)` 预申请。

更新后再合并相邻同值 VAL（长度 ≤4 才能并），保持紧凑。

### 升级 dense 的两个触发

1. 寄存器值 > 32（VAL 装不下）→ `goto promote`；
2. 超 `server.hll_sparse_max_bytes` 阈值 → `goto promote`。

`hllSparseToDense` 遍历 opcode 展开成 6-bit 数组，校验 `idx == HLL_REGISTERS` 否则判损坏。

---

## 十五、hllCount 估算公式：调和平均 + 两段校正

```c
E = (1/E) * alpha * m * m;         // ① 原始估计：α_m · m² · (Σ2^-ρ)⁻¹
if (E < m*2.5 && ez != 0)          // ② 小基数
    E = m * log(m/ez);             //    LinearCounting
else if (m==16384 && E < 72000)    // ③ 中段偏差
    E -= E * (bias/100);           //    多项式回归校正
```

- ① 分母 `Σ 2^(-ρj)` 由三个 Sum（hllDenseSum/hllSparseSum/hllRawSum）按编码分别算，`PE[64]` 打表省 pow。
- ② 小基数调和平均失效（零桶多导致高估），改用 LinearCounting 数空桶。
- ③ 4万~7.2万有经验系统性偏差，用论文拟合的多项式修正（仅 P=14）。

---

## 十六、缓存基数：card[8] 的 MSB 标志位

`card[8]` 存上次算好的基数（64 位小端），MSB 借作"缓存有效"标志：

```c
#define HLL_INVALIDATE_CACHE(hdr) (hdr)->card[0] |= (1<<7)   // MSB=1 失效
#define HLL_VALID_CACHE(hdr) (((hdr)->card[0] & (1<<7)) == 0) // MSB=0 有效
```

- PFADD 真改了寄存器 → `updated` → `HLL_INVALIDATE_CACHE` 置失效；
- PFCOUNT 先看 `HLL_VALID_CACHE`：为真直接读 8 字节，为假 `hllCount` 重算再写回。

### 为什么重算分支不显式"设为有效"

`hllCount` 返回的真实基数最高位必然为 0（HLL 基数远够不到 2^63），逐字节写回 card 时 card[0] 的 MSB 自动变 0，而 MSB=0 就是"有效"的定义。失效是主动置 1 的例外，有效是覆盖写回的默认。

### 写缓存前 dbUnshareStringValue + 标脏

重算会写回 card 字段，若字符串被共享会连累共享者，所以先 `dbUnshareStringValue`。且要 `signalModifiedKey` + `server.dirty++`，因为缓存被改了，不能当纯读命令。

---

## 十七、PFCOUNT 多 key 的 MAX 合并 + HLL_RAW

`PFCOUNT k1 k2` 语义是**并集的基数**，不是分别报两个。

### 合并原理：寄存器取 max

每个寄存器记"该桶最大 0 串"，并集后某桶元素 = 各 HLL 该桶元素的并集，其最大 0 串 = max(各 HLL 同桶值)。`hllMerge` 逐桶取 max，无需重哈希。

### HLL_RAW：内部专用编码

```c
#define HLL_RAW 255
```

多 key 合并用栈上 `uint8_t max[HLL_HDR_SIZE+HLL_REGISTERS]`，`registers` 是**1 字节 1 寄存器**的数组（非 6-bit 紧凑），标记 `HLL_RAW`。

```c
for (j = 1; j < c->argc; j++) {
    robj *o = lookupKeyRead(...);
    if (o == NULL) continue;        // 不存在的 key 当空 HLL
    hllMerge(registers,o);          // 逐桶 max 进数组
}
addReplyLongLong(c,hllCount(hdr,NULL));
```

### 为什么 RAW：省 CPU 换内存

RAW 下寄存器直读，且 `hllRawSum` 按 64 位字加速（整字全 0 则 8 个寄存器一起跳过）。用 16KB（1 字节/寄存器）换极速求和，与 dense 的 12KB 紧凑（6-bit）形成"省时 vs 省内存"的对偶。

### PFMERGE 的区别：持久化 + 强制 dense

PFMERGE 把合并结果**写回 dest key**，且 dest 强制转 dense（`hllSparseToDense` + `HLL_DENSE_SET_REGISTER` 写 6-bit 紧凑格式）。PFCOUNT 多 key 只是临时算并集，不落盘。
