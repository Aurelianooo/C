/**
 * 64位Linux操作系统，拥有root权限
 * 得到物理地址立即使用，不能存储
 * 注：COW触发时，父或子进程都有可能被分配新页
 */
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>

uint64_t virt_to_phys(void *virt, int *error)
{
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/pagemap", getpid());
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        *error = errno;
        return 0;
    }

    // 计算虚拟页号（页大小4K）和文件偏移量
    uint64_t pgoff = ((uint64_t)virt / 4096) * 8;
    uint64_t ent;
    if (lseek(fd, pgoff, SEEK_SET) < 0)
    {
        *error = errno;
        close(fd);
        return 0;
    }
    // 读取64位页表项
    ssize_t ret = read(fd, &ent, 8);
    close(fd);
    if (ret != 8)
    {
        *error = errno;
        return 0;
    }
    // 检查 page present bit
    if (!(ent & (1ULL << 63)))
    {
        *error = -1; // page not present
        return 0;
    }
    // 计算物理页框号，拼接得到物理地址
    uint64_t pfn = ent & ((1ULL << 55) - 1);
    *error = 0;
    return (pfn << 12) | ((uint64_t)virt & 0xfff);
}

void show(const char *label, void *addr, int val)
{
    int error = 0;
    uint64_t phys = virt_to_phys(addr, &error);
    printf("[%s] val=0x%08x, virt=%p", label, val, addr);

    if (error != 0)
    {
        if (error == -1)
        {
            printf(", phys not present\n");
        }
        else
        {
            printf(", phys error: %s\n", strerror(error));
        }
    }
    else
    {
        printf(", phys=0x%016lx\n", phys);
    }
}

volatile int g_val = 0x12345678;

int main()
{
    volatile int stack_val = 0xaaaaaaaa;
    volatile int *heap_val = malloc(sizeof(int));
    *heap_val = 0xbbbbbbbb;

    printf("--- BEFORE fork (parent) ---\n");
    printf("PID: %d\n", getpid());
    show("GLOBAL", (void *)&g_val, g_val);
    show("STACK ", (void *)&stack_val, stack_val);
    show("HEAP  ", (void *)heap_val, *heap_val);
    printf("\n");

    pid_t pid = fork();

    if (pid == 0)
    { // child
        printf("--- AFTER fork (child, BEFORE write) ---\n");
        printf("PID: %d\n", getpid());
        show("GLOBAL", (void *)&g_val, g_val);
        show("STACK ", (void *)&stack_val, stack_val);
        show("HEAP  ", (void *)heap_val, *heap_val);
        printf("\n");

        // 触发COW
        g_val = 0x11111111;
        stack_val = 0x22222222;
        *heap_val = 0x33333333;

        printf("--- AFTER fork (child, AFTER write) ---\n");
        printf("PID: %d\n", getpid());
        show("GLOBAL", (void *)&g_val, g_val);
        show("STACK ", (void *)&stack_val, stack_val);
        show("HEAP  ", (void *)heap_val, *heap_val);
        printf("\n");

        free((void *)heap_val);
        return 0;
    }

    wait(NULL); // parent
    printf("--- AFTER fork (parent) ---\n");
    printf("PID: %d\n", getpid());
    show("GLOBAL", (void *)&g_val, g_val);
    show("STACK ", (void *)&stack_val, stack_val);
    show("HEAP  ", (void *)heap_val, *heap_val);
    printf("\n");

    free((void *)heap_val);
    return 0;
}
