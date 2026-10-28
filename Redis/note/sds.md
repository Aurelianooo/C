# sds 模块源码阅读笔记

> Redis 3.0 的动态字符串库（SDS, Simple Dynamic String）。对外表现为普通 `char *`，背后携带长度信息，能 O(1) 取长度、二进制安全。

---

## 一、为什么 `sdshdr` 里用 `char buf[]` 而不是 `char *buf`？

### 两种写法的内存布局完全不同

**`char *buf`（指针成员）—— 两次分配，内存分散：**

```
sdshdr（一块内存）                  buf 数据（另一块内存）
┌──────────────┐                ┌─────────────────┐
│ len          │                │ 'h''e''l''l''o' │
│ free         │        ┌───►   │ '\0'            │
│ buf ─────────┼────────┘       │ ...             │
└──────────────┘                └─────────────────┘
```

**`char buf[]`（柔性数组）—— 一次分配，内存连续：**

```
一次 malloc 出来的整块连续内存
┌──────────┬──────────────────────┐
│ len(4)   │  buf(数据,紧跟其后)    │
│ free(4)  │  'h''e''l''l''o''\0'  │
└──────────┴──────────────────────┘
```

### 关键：SDS 的 O(1) 核心技巧靠连续布局

`sdslen`（`sds.h:67-70`）：

```c
static inline size_t sdslen(const sds s) {
    struct sdshdr *sh = (void*)(s-(sizeof(struct sdshdr)));
    return sh->len;
}
```

给定指向字符串的 `s`（buf 地址），**往前偏移 `sizeof(sdshdr)`** 拿到头，从而 O(1) 读到 `len`/`free`。这件事成立的前提只有 buf **紧贴结构体尾部、与 len/free 在同一块连续内存**。

如果 buf 是独立指针，这个偏移完全失效——数据在另一块内存，从 buf 地址往前减 8 拿不到 len。

### 为什么必须这样（而不是指针）

SDS 目标：对外像普通 C 字符串（就是一个 `char *`），但背后带长度信息。必须满足：

1. `sds` 仍是 `char *`，能直接传给所有接收 `char *` 的函数（printf/strlen/C 库），二进制兼容；
2. 从 `char *` 能 O(1) 反推长度，不用 O(n) 遍历。

柔性数组是唯一能同时满足这两点的常规手段。这也是 SDS 相比 C 原生字符串的核心优势。

### 一个关键认识：`&ptr` 和 `s - offset` 的区别

| 表达式 | 含义 | 类型 |
|--------|------|------|
| `ptr` | ptr 变量里存的值（一个地址） | `char *` |
| `*ptr` | 该地址指向的内容 | `char` |
| `&ptr` | ptr 变量本身的内存地址 | `char **` |

- `&ptr` **能**拿到指针本身的地址（类型 `char **`）。
- `s - 8` 既不是取内容也不是取自身地址，而是对指针**值**（字符串数据地址）做指针算术，只作用在"数据那块内存"附近。
- 指针版本里，"sdshdr" 和"数据"是两次 `malloc`、地址完全不相关，所以靠减偏移永远跳不回 len/free；只有连续布局（柔性数组）才能回溯。

### 一句话总结

不能用 `char *buf`，因为会让字符串数据分散到另一块内存，破坏"buf 紧贴结构体头部"的连续布局；SDS 的 O(1) `sdslen`/`sdsavail` 全靠 `s - sizeof(sdshdr)` 这个偏移反推头信息。`char buf[]`（柔性数组）正是用来保证"一次分配、连续存储、可反推"。

---

## 二、C99 之前如何实现柔性数组效果？

| 写法 | sizeof(sdshdr) | 是否标准 | 备注 |
|------|---------------|---------|------|
| `char buf[1]` | 12（含 padding） | 是（C89 可移植） | 最老派，多占 1 字节 |
| `char buf[0]` | 8 | 否（GCC 扩展） | 零长数组 |
| `char buf[]` | 8 | 是（C99 标准） | 现代推荐 |

- `char buf[1]`：声明成 1 元素数组，分配时 `malloc(sizeof(sdshdr) + initlen + 1)` 手动多开内存，多出来的紧跟 buf 之后，虚拟出"超长数组"。
- `char buf[0]`：GCC 扩展，标准不认可，但 `sizeof` 里不占字节。
- 柔性数组两个潜规则：① 必须是结构体最后一个成员；② 结构体不能只有这一个成员。

---

## 三、为什么 `sdslen`/`sdsavail` 写成 `static inline` 放 .h 里？

`static inline` 三个词各有用意：

- `inline` → 让编译器把函数体嵌入调用点，省掉 call/ret 开销；
- `static` → 避免多文件包含同一头文件导致的符号多重定义；
- 放 `.h` → inline 要求函数体在调用处可见，否则编译器无法展开。

**为什么偏偏是这两个函数**：它们"又短又热"——函数体就一次指针偏移 + 一次解引用，且是所有 SDS 操作的地基，被调用频率最高。太长的函数内联会膨胀代码伤指令缓存，太长不值得；低频函数内联收益可忽略。

判定是否内联的通用标准：**短小 + 高频 + 无副作用**。

---

## 四、`sdsMakeRoomFor` 的两个疑问

### 疑问 1：为什么不直接用 `sh->free`/`sh->len`？

看顺序（`sds.c:225-239`）：

```c
size_t free = sdsavail(s);   // 第 230 行，此时 sh 还没取出来
...
len = sdslen(s);             // 第 238 行
sh = (void*) (s-(sizeof(struct sdshdr)));  // 第 239 行，sh 这才出现
```

**不是"先有 sh 不用"，而是"读 free/len 时 sh 还没取"。** `sh` 是到第 239 行为了第 253 行的 `zrealloc` 才单独取的。

更深一层：`sdsavail`/`sdslen` 内部本身已经做了一遍 `sh = s - sizeof(sdshdr)`，作者用接口函数走"语义层"，只在真正要移动整个结构体（realloc）时才碰底层 `sh`，是一种分层写法。

### 疑问 2：为什么要做 `SDS_MAX_PREALLOC` 判断并扩容？

```c
newlen = (len + addlen);
if (newlen < SDS_MAX_PREALLOC)   // 小于 1MB
    newlen *= 2;                  // 翻倍
else
    newlen += SDS_MAX_PREALLOC;   // 大于等于 1MB，线性加 1MB
```

**这是"空间换时间"的预分配策略**（`SDS_MAX_PREALLOC = 1MB`）。

若每次都只分配"刚刚好"，每次 append 都要 realloc 并整体拷贝，总复杂度 O(N²)。多分配 1 倍（或 1MB），让后续很多次 append 直接落进 free，摊销成本降到 O(1)。

为什么用 1MB 分界：小字符串翻倍合理（总共没多少），大字符串再翻倍会浪费极巨内存（100MB 翻成 200MB），所以改用线性 +1MB，在"减少 realloc 次数"和"控制内存浪费"之间折中。

这就是 `free` 字段存在的意义——记录预分配出来还没用的空间；也是 `sdsavail` 高频出现的原因。

---

## 五、`sdsRemoveFreeSpace` 与预分配的关系

它是预分配的**反向操作**：预分配"提前多占"（扩张 free），它"事后回收"（收缩 free 到 0）。

```c
sh = zrealloc(sh, sizeof(struct sdshdr)+sh->len+1);
sh->free = 0;
```

把当初预分配出的 free 全部吐回系统。

**使用场景**：字符串"长成之后就不再动"、内存吃紧、长字符串要长期持有（如存入数据库的 value）。典型用法：先 `sdscat`/`sdsMakeRoomFor` 快速构建，构建完成后调它收尾。

**关键副作用**：`zrealloc` 可能搬地址，所以调用后原指针作废，必须用返回值接，且所有指向它的指针都要更新。

两个函数互补，形成"构建期要快（预分配）、持有期要省（回收）"的闭环。

---

## 六、为什么设计 `sdsfromlonglong`？

作用：把 `long long` 整数转成十进制字符串，存进 sds。

作者在注释里直接点明（`sds.c:621-624`）：它比 `sdscatprintf(sdsempty(),"%lld\n",value)` **快得多**。

**快的几个层次**：

1. `snprintf`/`printf` 是通用格式化引擎，要解析 fmt、处理对齐/宽度/精度、带 va_list；
2. `sdsfromlonglong` 直接调 `sdsll2str`，只做"手工取模循环 + 反转"，无 fmt 解析、无 va_list；
3. 用栈上定长数组 `buf[SDS_LLSTR_SIZE]`，零堆分配；
4. 最后 `sdsnewlen(buf, len)` 因为已知精确长度，一步到位精确分配。

核心答案：**性能** + 统一产出 sds 的一致性需求。

---

## 七、`SDS_LLSTR_SIZE = 21` 这个宏怎么算？

它不是算出来的，是硬编码的 `21`（`sds.c:560`），依赖"64 位有符号整数十进制最多 20 位数字 + 1 符号"这个事实。

**为什么是 20 位数字**：补码表示下正负不对称。

- 最大值 `2^63 - 1 = 9223372036854775807`，19 个数字；
- 最小值 `-2^63 = -9223372036854775808`，20 个数字。

负数的绝对值范围比正数多 1（`|−2^63| = 2^63` 比 `2^63 - 1` 大 1），所以位数上界由最小值决定：20 位数字 + 1 符号 = 21。

**一个边界小节**：`buf[21]` 里最长的负数表示（符号+20 位）需要写 21 个字符，末尾 `*p='\0'` 理论上落在索引 21（越界），这是历史实现里不严谨的地方，但调用方用返回的 `len` 而非 `\0`，日常不触发。

---

## 八、`sdscatvprintf` 在干嘛？

本质：用 printf 风格格式串把参数格式化成文本，追加到 sds 末尾。是 `sdscatprintf` 的底层。

**为什么需要 `va_list`**：标准库的 `vsnprintf` 只接受 `va_list`，`sdscatprintf` 负责 `va_start` 把 `...` 打包，`sdscatvprintf` 处理 va_list。

**核心难点：猜缓冲区大小（`sds.c:639-668`）**

1. **初始缓冲**：先试栈上 1024 字节 `staticbuf`（零堆分配）；若 `strlen(fmt)*2 > 1024` 则改堆分配。
2. **哨兵判满**：`buf[buflen-2] = '\0'`，`vsnprintf` 若输出填满导致截断，会把哨兵位覆盖成非 `\0`；检查 `buf[buflen-2] != '\0'` 即知"是否被截断"，O(1) 判满。
3. **翻倍重试**：被截断就 `va_copy(cpy, ap)`（每次循环要重新消费参数，因为 va_list 会被消耗）+ `buflen *= 2` + 重新 malloc，重试。
4. **拼接**：`sdscat(s, buf)` 追加，释放堆缓冲，返回新 sds。

**复杂性 O(N²)**：`sdscat` 内部可能 realloc 整体拷贝，反复调用会叠加成 O(N²)，所以注释强调别在热路径反复用格式化拼长字符串。

**与 `sdsfromlonglong` 的对比**：一个是结果长度未知必须动态试探，一个是长度上界已知可一步到位——后者因此更快。

---

## 九、`sdscatrepr` 的作用

把字符串转成"可读、可无损还原"的带引号转义形式，追加到 sds 末尾。等价于 Python 的 `repr()`。

三件事（`sds.c:1102-1128`）：

1. 首尾各加一个双引号；
2. 逐字节转义：

| 字符 | 转成 | 代码 |
|------|------|------|
| `\`、`"` | `\\`、`\"` | 1108-1110（两个 case 共享） |
| `\n`/`\r`/`\t`/`\a`/`\b` | `\n`/`\r`/`\t`/`\a`/`\b` | 1112-1116 |
| 可打印字符 | 原样 | 1118-1119 |
| 不可打印 | `\x%02x`（两位十六进制） | 1120-1121 |

**动机**：把含任意字节（`\0`、控制符、不可见字符）的缓冲区输出成安全、可读、可无损还原的文本。第一目标是**无损还原**（所以用 `\xNN` 而非丢弃），第二才是可读。

它和 `sdssplitargs` 互为逆操作，构成 Redis 命令协议编解码基础。

### 转义示例

输入 8 字节 `h i " \n NUL 0x01 \ 0x7F`，输出（带引号）：

```
"hi\"\n\x00\x01\\\x7f"
```

注意：`\x00` 不是空字节，而是 4 个字符 `\`、`x`、`0`、`0`；`\\` 是两个字符（字面反斜杠）。

### C 字符串转义易混点

`"\\n"` 只有 **2 个字节**（`\` + `n`），不是 3 个。关键：`\\` 是一个转义序列，代表一个字面反斜杠。

| 写法 | 字节数 | 内容 | 含义 |
|------|-------|------|------|
| `"\n"` | 1 | `0x0A` | 换行符 |
| `"\\n"` | 2 | `\` + `n` | 字面反斜杠+n |
| `"\\\n"` | 2 | `\` + `0x0A` | 反斜杠 + 换行符 |

---

## 十、`createEmbeddedStringObject` 与 EMBSTR 编码（object.c）

这是 SDS 柔性数组布局在 Redis object 层的一次"复用"，把 **robj + sdshdr + 字符串数据** 三样塞进一块连续内存。

```c
robj *createEmbeddedStringObject(char *ptr, size_t len) {
    robj *o = zmalloc(sizeof(robj)+sizeof(struct sdshdr)+len+1);
    struct sdshdr *sh = (void*)(o+1);
    ...
    o->ptr = sh+1;
    ...
    memcpy(sh->buf,ptr,len);
    sh->buf[len] = '\0';
}
```

### 三个指针偏移的含义

| 表达式 | 含义 |
|--------|------|
| 分配 `sizeof(robj)+sizeof(sdshdr)+len+1` | 一次 malloc 装下"对象头 + sds 头 + 字符串数据(\0)"三样 |
| `sh = o + 1` | 指针算术：跳过整个 robj，`sh` 指向紧跟其后的 sdshdr |
| `o->ptr = sh + 1` | 再跳过 sdshdr，指向 buf（字符串数据）起始 |

内存布局：

```
o      = &robj
sh=o+1 = &sdshdr（robj 之后）
sh+1   = &buf   （sdshdr 之后，即字符串数据）
o->ptr = sh+1   → 指向字符串数据 ✓
```

### 本质

用"一次 malloc + 指针偏移"替代"两次 malloc + 指针跳转"。这是 **EMBSTR 编码** 的核心：

- **RAW 编码**（`createRawStringObject`）：robj 和 sds 各一块内存，两次分配；
- **EMBSTR 编码**：全部内嵌一块内存，省一次分配、省一个指针跳转、内存局部性更好。

`createStringObject`（object.c:94）里长度 ≤ 39（`REDIS_ENCODING_EMBSTR_SIZE_LIMIT`）用 EMBSTR，否则用 RAW。39 是算出来的——让 `sizeof(robj)+sizeof(sdshdr)+len+1` 不超过 64 字节，恰好落入 jemalloc 的 64 字节 arena，享受最省内存、最快分配。

注意 `o->ptr` 指向 buf 而非 sdshdr 头，正与前文"`sds` 就是 buf 的地址、`sdslen` 靠 `s-sizefof(sdshdr)` 反推头"一致。这与 sds 柔性数组、zmalloc header 一脉相承：都在追求"元数据 + 数据塞进同一块连续内存"。
