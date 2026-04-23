/**
 * gcc mmap.c -lrt -std=c99 -O2
 * 该程序会在 /tmp 路径下生成一个 1G 大小的测试文件
 * 开启 O2 优化，这对减少循环开销、凸显 IO 性能差异至关重要
 *
 * read：每次随机访问都要发起 2 次系统调用（lseek + read），100 万次访问就是 200 万次上下文切换。
 * mmap：仅在初始化时调用 1 次 mmap，后续随机访问全是纯用户态内存操作，无任何系统调用开销。
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define TEST_FILE "/tmp/large_test_file.dat"
#define FILE_SIZE (1L * 1024 * 1024 * 1024) // 1GB 测试文件
#define BUF_SIZE (4096)                     // 页大小
#define RANDOM_CNT (1000000)                // 随机访问次数：100万次

int generate_test_file(const char *filename, off_t size)
{
    printf("正在生成 %lld MB 测试文件...\n", (long long)(size / (1024 * 1024)));

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        perror("open(generate)");
        return -1;
    }

    char *buf = malloc(BUF_SIZE);
    if (!buf)
    {
        perror("malloc");
        close(fd);
        return -1;
    }
    memset(buf, 'A', BUF_SIZE);

    off_t remaining = size;
    while (remaining > 0)
    {
        ssize_t to_write = (remaining > BUF_SIZE) ? BUF_SIZE : remaining;
        ssize_t written = write(fd, buf, to_write);
        if (written == -1)
        {
            perror("write");
            free(buf);
            close(fd);
            return -1;
        }
        remaining -= written;
    }

    free(buf);
    close(fd);
    printf("测试文件生成完成\n");
    return 0;
}

double test_read_random(const char *filename)
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        perror("open(read)");
        return -1.0;
    }

    char *buf = malloc(BUF_SIZE);
    if (!buf)
    {
        perror("malloc");
        close(fd);
        return -1.0;
    }

    unsigned long long checksum = 0;
    struct stat st;
    fstat(fd, &st);
    off_t file_size = st.st_size;

    srand(12345); // 固定随机种子，保证两次测试访问路径一致
    for (int i = 0; i < RANDOM_CNT; ++i)
    {
        off_t offset = (rand() % (file_size / BUF_SIZE)) * BUF_SIZE;
        lseek(fd, offset, SEEK_SET);
        read(fd, buf, BUF_SIZE);
        checksum += (unsigned char)buf[0];
    }

    free(buf);
    close(fd);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("[Read]  校验和: %llu | 耗时: %.3f 秒 | 速度: %.0f IOPS\n",
           checksum, elapsed, RANDOM_CNT / elapsed);
    return elapsed;
}

double test_mmap_random(const char *filename)
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        perror("open(mmap)");
        return -1.0;
    }

    struct stat st;
    fstat(fd, &st);
    off_t file_size = st.st_size;

    // 建立内存映射
    char *addr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return -1.0;
    }
    close(fd); // 映射建立后即可关闭文件描述符

    unsigned long long checksum = 0;
    srand(12345); // 使用相同的随机种子，保证公平对比
    for (int i = 0; i < RANDOM_CNT; ++i)
    {
        off_t offset = (rand() % (file_size / BUF_SIZE)) * BUF_SIZE;
        checksum += (unsigned char)addr[offset];
    }

    munmap(addr, file_size);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("[mmap]  校验和: %llu | 耗时: %.3f 秒 | 速度: %.0f IOPS\n",
           checksum, elapsed, RANDOM_CNT / elapsed);
    return elapsed;
}

int main()
{
    // 1. 检查并生成测试文件
    struct stat st;
    if (stat(TEST_FILE, &st) == -1 || st.st_size != FILE_SIZE)
    {
        if (generate_test_file(TEST_FILE, FILE_SIZE) == -1)
            return 1;
    }

    // 2. 预热
    printf("--- 预热阶段 ---\n");
    test_mmap_random(TEST_FILE);
    printf("\n");

    // 3. 正式测试
    printf("--- 随机访问性能对比 (100万次 IO) ---\n");
    double t_read = test_read_random(TEST_FILE);
    double t_mmap = test_mmap_random(TEST_FILE);

    return 0;
}
