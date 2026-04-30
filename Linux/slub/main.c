#include "slub_mm.h"
#include <stdio.h>
#include <string.h>

int main()
{
    // 初始化
    if (slub_init() != 0)
    {
        return -1;
    }

    printf("\n--- 开始测试 SLUB malloc/free ---\n");

    // 测试分配
    void *p0 = slub_malloc(10);   // 16字节
    void *p1 = slub_malloc(10);   // 16字节
    void *p2 = slub_malloc(100);  // 128字节
    void *p3 = slub_malloc(512);  // 512字节
    void *p4 = slub_malloc(1024); // 1024字节

    printf("分配成功: p1=%p p2=%p p3=%p p4=%p\n", p1, p2, p3, p4);

    // 测试读写, 注意不要越界!!!
    strcpy(p1, "SLUB Test");
    strcpy(p2, "Alloc/Free Test");
    printf("写入数据: p1=%s\np2=%s\n", p1, p2);

    // 测试释放
    slub_free(p1);
    slub_free(p2);
    printf("释放 p1, p2 成功\n");

    // 测试复用内存
    void *p5 = slub_malloc(16);
    void *p6 = slub_malloc(128);
    printf("复用内存: p5=%p p6=%p\n", p5, p6);

    // 测试非法操作
    slub_malloc(1030);
    slub_free((void *)0x1234);

    // 清理
    slub_free(p0);
    slub_free(p3);
    slub_free(p4);
    slub_free(p5);
    slub_free(p6);

    printf("--- 测试完成 ---\n\n");

    slub_destroy();
    return 0;
}
