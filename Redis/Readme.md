# Redis 3.0 源码阅读路线

> 按由底向上、由数据结构到系统的顺序组织。每个阶段先读底层结构，再读基于它们实现的上层模块。

---

## 第一阶段：数据结构

> Redis 的基础数据结构，位于独立的源文件中，是后续所有上层类型的基石。

| 模块 | 文件 |
|------|------|
| 内存分配 | `zmalloc.c`、`zmalloc.h` |
| 动态字符串 | `sds.h`、`sds.c` |
| 双端链表 | `adlist.c`、`adlist.h` |
| 字典 | `dict.h`、`dict.c` |
| 跳跃表 | `redis.h` 中的 `zskiplist` / `zskiplistNode` 结构，`t_zset.c` 中所有 `zsl` 开头的函数（如 `zslCreate`、`zslInsert`、`zslDeleteNode` 等） |
| 基数统计 | `hyperloglog.c` 中的 `hllhdr` 结构，以及所有 `hll` 开头的函数 |

---

## 第二阶段：内存编码结构

> Redis 为了省内存引入的特殊紧凑编码，是对象系统的底层存储。

| 模块 | 文件 |
|------|------|
| 整数集合 | `intset.h`、`intset.c` |
| 压缩列表 | `ziplist.h`、`ziplist.c` |

---

## 第三阶段：数据类型实现

> 对象系统封装底层编码，向上提供五大数据类型。每个 `t_*.c` 都是一类键的命令实现。

| 模块 | 文件 |
|------|------|
| 对象系统 | `object.c` |
| 字符串键 | `t_string.c` |
| 列表键 | `t_list.c` |
| 散列键 | `t_hash.c` |
| 集合键 | `t_set.c` |
| 有序集合键 | `t_zset.c`（除 `zsl` 开头函数之外的全部函数） |
| HyperLogLog 键 | `hyperloglog.c` 中所有 `pf` 开头的函数 |

---

## 第四阶段：数据库实现

> 从"单个类型"上升到"数据库整体"，以及持久化与部分独立功能模块。

| 模块 | 文件 |
|------|------|
| 数据库实现 | `redis.h` 中的 `redisDb` 结构，`db.c` |
| 通知功能 | `notify.c` |
| RDB 持久化 | `rdb.c` |
| AOF 持久化 | `aof.c` |
| 发布与订阅 | `redis.h` 中的 `pubsubPattern` 结构，`pubsub.c` |
| 事务 | `redis.h` 中的 `multiState`、`multiCmd` 结构，`multi.c` |

---

## 第五阶段：客户端与服务器端

> 事件驱动框架、网络层与服务器主程序，之后可继续阅读若干独立功能模块。

### 事件与网络

| 模块 | 文件 |
|------|------|
| 事件处理 | `ae.c`、`ae_epoll.c`、`ae_evport.c`、`ae_kqueue.c`、`ae_select.c` |
| 网络链接 | `anet.c`、`networking.c` |
| 服务器端 | `redis.c` |
| 客户端 | `redis-cli.c` |

### 独立功能模块

| 模块 | 文件 |
|------|------|
| Lua 脚本 | `scripting.c` |
| 慢查询 | `slowlog.c` |
| 监视 | `monitor.c` |

---

## 第六阶段：多机实现

> Redis 在多机 / 分布式场景下的代码实现。

| 模块 | 文件 |
|------|------|
| 复制功能 | `replication.c` |
| Redis Sentinel | `sentinel.c` |
| 集群 | `cluster.c` |
