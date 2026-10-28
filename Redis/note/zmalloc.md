# zmalloc 模块源码阅读笔记

> Redis 3.0 内存分配封装模块。核心目标：在系统 `malloc` 之上，额外维护一个精确的 `used_memory` 计数器，供 `INFO memory` 等使用。

---

## 一、为什么 `zlibc_free` 定义在 `#include "zmalloc.h"` 之前？

### 疑问来源

`zlibc_free` 这个函数定义在 `#include "zmalloc.h"` **之前**（`zmalloc.c:38`），即使有注释也很难直接看懂。

### 核心原因：`free` 是个宏

看 `zmalloc.c` 的这组宏（第 58-68 行）：

```c
#if defined(USE_TCMALLOC)
#define malloc(size) tc_malloc(size)
...
#define free(ptr) tc_free(ptr)      // 把 free 重定义
#elif defined(USE_JEMALLOC)
#define free(ptr) je_free(ptr)      // 把 free 重定义
#endif
```

这些宏是**预处理阶段的文本替换**。一旦 `#define free(ptr) ...` 出现，**它之后源码里出现的所有 `free(...)` 都被替换**成 `tc_free(...)` / `je_free(...)`。

而 `zmalloc.h` 里 `#include <google/tcmalloc.h>` / `#include <jemalloc/jemalloc.h>`（第 40/50 行）正是触发这些宏的地方 —— `zmalloc.h` 自己没写 `#define free`，是 jemalloc/tcmalloc 的头文件把 `free` 重定义了。

### 顺序为什么重要

如果 `zlibc_free` 写在 `#include "zmalloc.h"` **之后**：

```c
#include "zmalloc.h"          // free 已被重定义

void zlibc_free(void *ptr) {
    free(ptr);                // 会变成 je_free(ptr) / tc_free(ptr)
}
```

那它就不再是 libc 原始 `free` 了。

而现在写在**前面**，此时还没包含 `zmalloc.h`，`free` 还是 `<stdlib.h>` 里的原始 `free`，所以 `zlibc_free` 里的 `free(ptr)` 不会被替换，被"固化"下来。

### 为什么需要这个"原始 free"

注释点明：用于释放 `backtrace_symbols()` 返回的结果。

`backtrace_symbols()` 是 glibc 自己用 libc 的 `malloc` 分配内存、返回字符串数组，**必须用 libc 的 `free` 去释放**，不能用 `zfree`，因为：

1. `zfree` 会根据 `PREFIX_SIZE` 往前偏移找 header（`zmalloc.c:203`），而 `backtrace_symbols` 返回的指针没有这个 header，会越界。
2. 一旦用了 jemalloc/tcmalloc，它们的 `free` 与 libc 的 `malloc` 不配对。

所以只要是从 **libc 而非 zmalloc 分配来的内存**，就用 `zlibc_free`（绕过宏、直通原始 free）来释放。

### 顺带一提：zlibc_free 在 3.0 里未被调用

全源码搜索，`zlibc_free` 只有两处：定义（`zmalloc.c:38`）和声明（`zmalloc.h:79`），**没有任何调用点**。它是一个：

- 非 static、带外部链接的**公开导出接口**，为外部/未来代码预留；
- 防御性设计：固化"原始 free"，防止未来"用 `zfree` 释放 libc 内存"这类 bug；
- 在官方较新版本的 Redis 中，这个函数确实有调用点（在 crash 打印 backtrace 处）。

### 一句话总结

`free` 在包含 jemalloc/tcmalloc 头文件后会被 `#define` 成 `je_free`/`tc_free`，而 `zlibc_free` 想要的是 libc 原生 `free`，所以必须在 `#include "zmalloc.h"` **之前**定义，趁 `free` 还是原生时把它用掉。

---

## 二、`HAVE_MALLOC_SIZE` 宏是干嘛的？

### 核心意义

zmalloc 需要"给一个指针，查出这块内存在分配时实际占了多少字节"的能力（用于统计 `used_memory`，以及 `zfree`/`zrealloc` 扣减正确数量）。

这个能力有两种来源：

- **分配器自带**（如 `malloc_usable_size` / `malloc_size`）→ 定义 `HAVE_MALLOC_SIZE`
- **分配器不带** → zmalloc 自己多存一个 header 记录大小

`HAVE_MALLOC_SIZE` 就是"前者成立"的标志。

### 谁定义它（`zmalloc.h` 第 38-62 行）

```c
#if defined(USE_TCMALLOC)
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) tc_malloc_size(p)
#elif defined(USE_JEMALLOC)
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) je_malloc_usable_size(p)
#elif defined(__APPLE__)
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) malloc_size(p)
#endif
```

### 它如何改变行为

| 路径 | `PREFIX_SIZE` | 分配方式 |
|------|--------------|---------|
| 有 `HAVE_MALLOC_SIZE` | `0` | 直接 `malloc(size)`，返回原指针，大小靠 `zmalloc_size()` 现查 |
| 无 `HAVE_MALLOC_SIZE` | `sizeof(size_t)` | `malloc(size + PREFIX_SIZE)`，头部写 size，返回偏移后指针 |

### 为什么需要这个区分

glibc 早期没有标准可移植的 `malloc_usable_size` 等价物（它是 GNU 扩展）。jemalloc/tcmalloc 都提供了 `malloc_usable_size`，能直接精确返回实际分配大小。

### 一句话总结

`HAVE_MALLOC_SIZE` = "分配器能否直接查大小"。是 zmalloc 两条实现路径的分叉点，决定了 `PREFIX_SIZE`、`zmalloc()`、`zfree()`、`zrealloc()`、`zmalloc_size()` 的全部走向。

---

## 三、`update_zmalloc_stat_alloc` 宏在干嘛？

### 职责

一句话：**在每次分配后，把实际消耗的字节数累加到全局计数器 `used_memory`**。

```c
#define update_zmalloc_stat_alloc(__n) do { \
    size_t _n = (__n); \
    if (_n&(sizeof(long)-1)) _n += sizeof(long)-(_n&(sizeof(long)-1)); \
    if (zmalloc_thread_safe) { \
        update_zmalloc_stat_add(_n); \
    } else { \
        used_memory += _n; \
    } \
} while(0)
```

### 1. 对齐（关键）

```c
if (_n&(sizeof(long)-1)) _n += sizeof(long)-(_n&(sizeof(long)-1));
```

malloc 分配时几乎总把大小向上对齐到边界（通常 8/16 字节）。这行把 `_n` **向上取整到 `sizeof(long)`（一般 8）的倍数**：

- `_n & (sizeof(long)-1)` = 取 `_n` 除以 8 的余数；
- 余数非 0，则加 `8 - 余数` 凑成整数倍。

例：`_n = 13` → 余 5 → `13 + 3 = 16`；`_n = 16` → 余 0 → 不动。

**目的**：让 `used_memory` 记录"真实占用"而非"用户请求"，这样才与 RSS 对得上，碎片率才算得准。

配对的 `update_zmalloc_stat_free` 做**完全相同**的对齐再扣减，保证加和减用同一套口径。

### 2. 线程安全分支

```c
if (zmalloc_thread_safe) {
    update_zmalloc_stat_add(_n);   // 原子/加锁
} else {
    used_memory += _n;             // 裸操作
}
```

Redis 默认**单线程**，不加锁省性能；后台线程（`bio.c` 等）通过 `zmalloc_enable_thread_safeness()` 开启线程安全。

`update_zmalloc_stat_add` 又分两种（第 70-86 行）：

- 有 `HAVE_ATOMIC` → gcc 内建 `__sync_add_and_fetch` 原子操作；
- 否则 → `used_memory_mutex` 互斥锁保护。

### 3. `do { ... } while(0)` 是什么

C 宏惯用技巧：让宏展开后无论放在哪里都表现为一条完整语句，`while(0)` 保证只执行一次，避免 `if/else` 悬挂问题。

### 一句话总结

`update_zmalloc_stat_alloc` = "把本次分配的真实内存量（先对齐到 `sizeof(long)` 倍数）累加进全局 `used_memory`，根据线程安全与否选择原子/加锁方式"。与 `update_zmalloc_stat_free` 是一对，一个加、一个减，共用同一套对齐逻辑。

---

## 四、为什么"ptr 之前的 size"这么重要？一定要这样实现吗？

### 这是无 `HAVE_MALLOC_SIZE` 路径下的 header 机制

普通 glibc 环境下 `free`/`realloc` 不需要你告诉 size（glibc 内部自己记账），但 **zmalloc 自己需要 size**，因为它要维护 `used_memory`：

```c
void zfree(void *ptr) {
    ...
    realptr = (char*)ptr - PREFIX_SIZE;      // 回退到真正 malloc 的头
    oldsize = *((size_t*)realptr);           // 读出当时记录的 size
    update_zmalloc_stat_free(oldsize + PREFIX_SIZE); // 精确扣减
    free(realptr);
}
```

**这个 header 里的 size 存在的唯一理由，就是 glibc 不告诉你 size，Redis 得自己记住。** 它是 `used_memory` 精确统计的唯一数据来源。

### 为什么重要到这种程度 —— 这是正确性，不是优化

`zfree` 时回退读 size 是**唯一的 size 来源**。如果读错或越界：

1. 扣减数错误 → 内存统计漂移，`used_memory` 与真实 RSS 对不上；
2. 读越界 → 在"有 header"的指针上用了"无 header"的释放（或反过来），可能 segfault。

这正是 `zlibc_free` 那个坑的来源：`free` 和被释放的指针必须是同一套布局。

### "一定要这样实现吗" —— 不一定

Redis 给了两条路，header 只是其中一条（兜底）：

| 场景 | 实现 |
|------|------|
| 有 `malloc_usable_size`（jemalloc/tcmalloc/macOS） | **不存 header**，`PREFIX_SIZE=0`，每次现查 size |
| 没有（纯 glibc 3.0 时代） | **自己存 header**，指针前 `PREFIX_SIZE` 字节写 size |

所以：**不是"一定要"，而是"没有系统查询接口时不得不"**。

作者 antirez 其实很讨厌 header 方案：每次分配多花 8 字节、指针偏移易出 bug、与 glibc 内部记账重复。所以 Redis 强烈推荐用 jemalloc —— 用了它 `HAVE_MALLOC_SIZE` 生效，整套 header 机制全部不编译、直接消失。

### 第三种方案（拓展）

1. 干脆不维护 `used_memory`，直接用系统 `malloc_stats` / `/proc` 读 RSS（Redis 的 `zmalloc_get_rss` 就是走 `/proc/self/statm`），但 RSS 包含分配器碎片膨胀，不如应用层精确统计利于分析。
2. 包装返回结构，本质上还是 header，只是换个写法。
3. 用 `malloc_usable_size` 兜底 —— 即方案 A，已是首选。

### 一句话总结

这个 size **重要**，因为它是 `used_memory` 精确统计的唯一数据来源（glibc 的 free 不需要、也不提供 size）；但**不是"一定要"** —— 分配器提供 `malloc_usable_size` 时，Redis 会抛弃这套 header（即 `HAVE_MALLOC_SIZE` 路径），只是默认 glibc 没有这个接口才不得不做。本质是"分配器能力不同 → 换实现"的分支设计。
