# adlist 模块源码阅读笔记

> Redis 3.0 的通用双端链表（doubly linked list）。用 `void *` 存值 + 函数指针回调实现"多态"，能存任意类型。

---

## 一、数据结构

- **listNode**（节点）：`prev` / `next` / `void *value`
- **list**（链表）：`head` / `tail` / `len`，加上三个函数指针 `dup` / `free` / `match`
- **listIter**（迭代器）：`next`（当前迭代到的节点）/ `direction`（方向）

宏封装的 O(1) 访问器：`listLength`/`listFirst`/`listLast`/`listPrevNode`/`listNextNode`/`listNodeValue` 等。

---

## 二、为什么没有"空的头节点"？

### 概念区分：带头节点 vs 不带头节点

**带头节点（哨兵/dummy node）**：教材经典写法，链表里有个不存数据、只占位的节点，`head` 永远指向它。

```
┌───────┐    ┌────┐    ┌────┐
│ 头节点 │──►│数据1│──►│数据2│──► NULL
│(空)   │◄──│    │◄──│    │
└───────┘    └────┘    └────┘
```

**不带头节点（Redis 的做法）**：`head` 直接指向第一个真实数据节点，空链表时 `head == tail == NULL`。

```
┌────┐    ┌────┐    ┌────┐
│数据1│◄─►│数据2│◄─►│数据3│
└────┘    └────┘    └────┘
  head ──────────────► tail
```

### 验证代码

- `listCreate`（`adlist.c:57`）：`list->head = list->tail = NULL;`
- `listAddNodeHead` 空链表分支：`list->head = list->tail = node;`

都是让 head/tail 指向**数据节点本身**，没有哨兵。

### 为什么 Redis 不带头节点

带头节点的好处是插入/删除时省掉"空链表/头尾"特判，代码路径统一。但代价是要多占一个节点内存、遍历要跳过哨兵、对外语义不直观。

Redis 是性能极致 + 内存敏感的系统，用「head/tail 双指针 + len 计数」的闭环双端链表设计，插入删除靠改 prev/next 指针 O(1)，空链表用 `if (len == 0)` 一个特判就解决，不需要哨兵。

### 一个连带容易混淆的点

`head`/`tail` 是 list 结构体里的**两个指针字段**（Redis 有）；"头节点/哨兵"是链表里**实际存在但不装数据的节点**（Redis 没有）。二者不是一回事。

---

## 三、为什么要设计 `listIter` 迭代器？

### 不用迭代器确实能遍历

```c
listNode *node = list->head;
while (node) {
    doSomething(listNodeValue(node));
    node = node->next;
}
```

单向单层遍历时裸指针完全够用，甚至更快。

### 迭代器解决的三个问题

1. **方向抽象**：`AL_START_HEAD` / `AL_START_TAIL`。不封装就得写"正向"和"反向"两套 while 循环；封装后遍历代码不变，只在 `listGetIterator(list, 方向)` 决定方向。把"遍历逻辑"和"怎么走到下一个"解耦。

2. **删除安全**：`listNext` 里先 `iter->next = current->next`（**先存下一个再返回当前**），所以调用方拿到 `current` 后哪怕删掉它，下次 `listNext` 仍能返回正确节点。裸指针遍历 + 删除会导致 UAF（free 后还 `node->next`）。

3. **统一接口**：Redis 里 dict 有 `dictIterator`、ziplist 有 `zlist` 等，统一用 `getIterator() + while(next(iter))` 形态，风格一致。

### 代价

多一次 zmalloc/zfree、多一层函数调用、每次 `direction` 判断。所以不适合极热内层循环。

### 权衡的体现：`listRotate` 反例

`listRotate` 没有用迭代器，直接 `list->tail` / `tail->prev` 操作——因为它是**定点操作**（取尾移头）而非遍历。体现"定点操作用指针，通用遍历用迭代器"。

---

## 四、`listRotate` 有什么用？

### 它做的是"旋转一步"，不是反转

```
旋转前: A → B → C → D → E
                     ↑tail
旋转后: E → A → B → C → D
        ↑head           ↑tail
```

`tail` 节点 E 从尾部摘下移到头部，其余顺序不变，整体往右轮转一个位置。连续 N 次转一圈回到原状。

### 用途：轮询调度（round-robin）

让链表当"环形轮询器"用：每调用一次，"当前对象"移到末尾，"下一个对象"浮到队首，配合一直取 `list->head` 实现轮流。

Redis 实际场景：

1. **客户端超时扫描**：为公平检查所有连接是否空闲超时，不能每次都从头扫到尾（靠后的连接容易被跳过），用 `listRotate` 把刚处理的客户端转到队尾，保证每个都被轮到。
2. **就绪 key 队列**：阻塞操作（如 BLPOP）里就绪的 key 队列公平轮转。
3. 通用"循环处理同一批对象"的做法。

### 为什么不用反转或取模

- 反转是 O(N) 且语义完全不同（打乱相对顺序）；
- 取模索引对链表是 O(N) 慢速；rotate 是 O(1)，天然适配链表。

**一句话**：`listRotate` 用双端链表"摘尾、插头都是改几个指针"的 O(1) 特性，实现公平 round-robin。
