# t_hash.c 模块问答

> 对应源码:`src/t_hash.c`
> 相关文件:`src/dict.c`、`src/ziplist.c`、`src/object.c`

---

## 一、t_hash.c 是什么?

实现 Hash 类型(key → field → value 映射)。结构和 t_list.c 高度一致,分两部分:

1. **上半部分(33~700 行):Hash type API** —— `hashTypeXxx` 抽象两种编码差异。
2. **下半部分(705 行起):Hash type commands** —— 具体命令。

### 两种底层编码

| 编码 | 底层结构 | 适用 |
|------|---------|------|
| ZIPLIST | ziplist | 小哈希(field-value 对少且短) |
| HT | dict | 大哈希 |

两个阈值:`hash-max-ziplist-entries`(默认 512)、`hash-max-ziplist-value`(默认 64)。

### 核心差异:field/value 成对交错存放

Hash 用 ziplist 时,field 和 value 交替存放,一个 field-value 对占两个相邻节点:

```
ziplist: [field1][value1][field2][value2]...
```

故 `hashTypeLength` 写 `ziplistLen(o->ptr) / 2`。

---

## 二、hashTypeSet 的两种编码分派

```c
int hashTypeSet(robj *o, robj *field, robj *value) // 返回 update:0=新增,1=更新
```

### ZIPLIST 分支:field/value 成对操作

```c
field = getDecodedObject(field);   // ① 解码(ziplist 只存字节)
value = getDecodedObject(value);

fptr = ziplistIndex(zl, ZIPLIST_HEAD);
fptr = ziplistFind(fptr, field->ptr, sdslen(field->ptr), 1);  // ② skip=1 跳过value找field
if (fptr != NULL) {
    vptr = ziplistNext(zl, fptr);   // ③ value 在 field 下一个节点
    update = 1;
    zl = ziplistDelete(zl, &vptr);           // ④ 删旧 value
    zl = ziplistInsert(zl, vptr, value->ptr, sdslen(value->ptr)); // ⑤ 插新 value
}
if (!update) {
    zl = ziplistPush(zl, field->ptr, ..., ZIPLIST_TAIL);  // ⑥ 成对 push
    zl = ziplistPush(zl, value->ptr, ..., ZIPLIST_TAIL);
}
decrRefCount(field); decrRefCount(value);   // ⑦ 释放临时
```

关键点:

- **skip=1**:ziplist 里 field/value 交替,查找 field 要跳过 value 每隔一个比一次(区别于 List 挨个比)。
- **删旧 value 插新 value**:更新时 field 不变,只需原位替换 value 节点。
- **新增**:把 field、value 依次 push 到尾部,保持配对顺序。

### HT 分支:dictReplace 返回值语义

```c
if (dictReplace(o->ptr, field, value)) { /* Insert */
    incrRefCount(field);
} else { /* Update */
    update = 1;
}
incrRefCount(value);
```

**注意 dictReplace 返回值与直觉相反**:新增返回 1,替换返回 0。所以返回真→新增→field incrRefCount;返回假→替换→update=1。value 无论新增替换都 incrRefCount(dict 一定新持有它)。

`hsetCommand`:update==1 回 0(覆盖),update==0 回 1(新增)。

---

## 三、hincrbyfloatCommand 的复制/AOF 一致性

与 t_string 的 `incrbyfloatCommand` 同款,但有三处差异:

```c
value += incr;
new = createStringObjectFromLongDouble(value);
hashTypeTryObjectEncoding(o,&c->argv[2],NULL);
hashTypeSet(o,c->argv[2],new);
...
aux = createStringObject("HSET",4);      // ① 改写成 HSET,4 字符
rewriteClientCommandArgument(c,0,aux);
decrRefCount(aux);
rewriteClientCommandArgument(c,3,new);    // ② 替换下标 3(不是 2)
decrRefCount(new);                        // ③ 结尾多了 decrRefCount
```

三处差异原因:

1. **命令名 HSET、下标 3**:`HINCRBYFLOAT key field incr` 中 incr 在 argv[3](而 INCRBYFLOAT 的 increment 在 argv[2])。
2. **多了 hashTypeTryObjectEncoding**:hash 有 field/value 两维度,写回前再压一次编码。
3. **多了 decrRefCount(new)**:`hashTypeSet` 内部对 value incrRefCount 了,函数内临时引用要平衡释放(而 t_string 走 dbOverwrite,记账不同)。

核心动机一致:传播确定性结果而非非确定性操作,避免浮点精度/格式化在主从、AOF 恢复时不一致。

---

## 四、hashTypeConvertZiplist 的搬迁 + tryObjectEncoding

ziplist → HT 的搬迁函数:

```c
hi = hashTypeInitIterator(o);
dict = dictCreate(&hashDictType, NULL);
while (hashTypeNext(hi) != REDIS_ERR) {
    field = hashTypeCurrentObject(hi, REDIS_HASH_KEY);
    field = tryObjectEncoding(field);      // ★ 再编码省内存
    value = hashTypeCurrentObject(hi, REDIS_HASH_VALUE);
    value = tryObjectEncoding(value);      // ★ 再编码
    ret = dictAdd(dict, field, value);
    if (ret != DICT_OK) {
        redisLogHexDump(...);              // 数据损坏,打日志
        redisAssert(ret == DICT_OK);       // 断言
    }
}
hashTypeReleaseIterator(hi);
zfree(o->ptr);                             // 释放旧 ziplist
o->encoding = REDIS_ENCODING_HT;
o->ptr = dict;
```

### 关键:顺手 tryObjectEncoding 压编码

`hashTypeCurrentObject` 对字符串用 `createStringObject`(可能还是 RAW/EMBSTR),未走"转整数"。所以搬迁时再 `tryObjectEncoding` 一次,能把"123"这样的字符串 field/value 转成 INT 编码的共享整数,避免搬迁丢失省内存机会。

相比 t_list 的 `listTypeConvert`,这里多了 tryObjectEncoding,是 hash"双维度 + 字典 key 编码敏感"的特有优化。

### 防御:重复 field

正常 field 唯一,若 ziplist 有重复(dictAdd 返回 DICT_ERR),logHexDump dump 原始字节 + assert 终止 —— 重复 field 是内部 bug/数据损坏,非用户输入,用断言兜底。

---

## 五、hashTypeNext 的 field/value 成对遍历

```c
int hashTypeNext(hashTypeIterator *hi) {
    if (hi->encoding == REDIS_ENCODING_ZIPLIST) {
        fptr = hi->fptr; vptr = hi->vptr;
        if (fptr == NULL) {
            fptr = ziplistIndex(zl, 0);      // 首次:定位下标0(第一个field)
        } else {
            fptr = ziplistNext(zl, vptr);    // 后续:从上一个value跳到下一个field
        }
        if (fptr == NULL) return REDIS_ERR;  // 空或到底
        vptr = ziplistNext(zl, fptr);         // 本对value在field后一格
        redisAssert(vptr != NULL);
        hi->fptr = fptr; hi->vptr = vptr;
    } else if (REDIS_ENCODING_HT) {
        if ((hi->de = dictNext(hi->di)) == NULL) return REDIS_ERR;
    }
    return REDIS_OK;
}
```

核心:用 `fptr`/`vptr` 两个指针跟踪一对 field-value,从 value 前进一格就是下一个 field(因为严格交替)。

`hashTypeCurrentFromZiplist` 用 `REDIS_HASH_KEY/VALUE` 标志选读 `fptr`(field)还是 `vptr`(value)。dict 分支则一个 `dictNext` 搞定(entry 自带 key+value)。

### field/value 节点大小固定吗?

**不固定**,都是变长的。ziplist 里每个节点是 `<prevlen><encoding><data>`,prevlen 和 encoding 都动态编码,内容长短各异。

遍历靠每个节点头部记录的信息:`ziplistNext` 用 `zipRawEntryLength` 算出当前节点占多少字节跳过去。

推论:变长 + 头部记长度 → prevlen 级联更新(ziplist 著名的 O(N²) 坑),这正是 hash 设阈值、超了升级 dict 的原因。

---

## 六、hashTypeDelete 的 dict 收缩

```c
} else if (o->encoding == REDIS_ENCODING_HT) {
    if (dictDelete((dict*)o->ptr, field) == REDIS_OK) {
        deleted = 1;
        if (htNeedsResize(o->ptr)) dictResize(o->ptr);   // 删完检查收缩
    }
}
```

### htNeedsResize 判断什么

哈希表大但元素少(使用率 < 10%,且 size > 初始大小)时,该收缩。

### 为什么收缩要放上层,而非底层 dictDelete

`dictDelete` 底层只管"删一个 entry",不做缩表判断。原因:

1. **防抖动(flapping)**:删太频繁,每次删都缩会反复缩-扩,rehash 大量搬迁。
2. **职责分离**:何时缩是上层策略。

故 `hashTypeDelete` 删完主动 `htNeedsResize` + `dictResize` 收尾。

### dictGenericDelete 第 3 参数与缩表无关

`dictDelete` 调 `dictGenericDelete(d, key, 0)`,第 3 参是 `nofree`(是否释放 key/value),与缩表无关。`dictDeleteNoFree` 传 1(只摘不释放)。

### 扩容 vs 收缩

- 扩容在插入路径 `_dictExpandIfNeeded` 自动做(负载 > 1),且受 `dict_can_resize`(BGSAVE 时禁用)约束。
- 收缩释放内存,无 BGSAVE 顾虑,但阈值压很低(负载 < 10%)防抖动,由上层显式触发。

---

## 附:为什么表删了不会自动变小

底层 `dictDelete` 刻意不做缩表判断——删元素太频繁,每次删都缩会抖动,且"何时缩"是上层策略。`hashTypeDelete` 里 `htNeedsResize` + `dictResize` 正是为此补上的收尾动作。
