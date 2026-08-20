/* fcntl() 记录锁演示：字节范围锁

   编译: gcc -std=c89 -Wall -pedantic fcntl_lock.c -o fcntl_lock
   运行: ./fcntl_lock <共享文件路径>

   场景：
     两个子进程分别写文件的前半段和后半段，通过 fcntl 记录锁
     各自锁定自己写的区间，互不干扰，展示字节范围锁的粒度优势。

     另外用 F_GETLK 演示如何探测"某段区间被谁锁着"。
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#define FILE_SIZE 1024

/* 对 [offset, offset+len) 区间加锁。cmd 用 F_SETLKW 阻塞、F_SETLK 非阻塞 */
static int lock_range(int fd, int type, off_t start, off_t len, int blocking)
{
    struct flock fl;

    memset(&fl, 0, sizeof(fl));
    fl.l_type   = type;                /* F_RDLCK / F_WRLCK / F_UNLCK */
    fl.l_whence = SEEK_SET;
    fl.l_start  = start;
    fl.l_len    = len;
    fl.l_pid    = 0;

    if (blocking)
        return fcntl(fd, F_SETLKW, &fl);
    else
        return fcntl(fd, F_SETLK, &fl);
}

/* 探测 [start, start+len) 区间是否被锁，返回持锁进程 PID（0 表示无锁） */
static pid_t probe_range(int fd, off_t start, off_t len)
{
    struct flock fl;

    memset(&fl, 0, sizeof(fl));
    fl.l_type   = F_WRLCK;             /* 假装要加写锁来探测 */
    fl.l_whence = SEEK_SET;
    fl.l_start  = start;
    fl.l_len    = len;

    if (fcntl(fd, F_GETLK, &fl) == -1) {
        perror("fcntl F_GETLK");
        return -1;
    }

    if (fl.l_type == F_UNLCK)
        return 0;                      /* 无锁 */
    return fl.l_pid;                   /* 被这个进程锁着 */
}

/* 写者：锁住自己的区间，慢速写入模拟 IO */
static void writer(int fd, const char *who, off_t start, off_t len)
{
    int i;
    char c = (who[0] == 'A') ? 'A' : 'B';

    for (i = 0; i < 5; i++) {
        /* 阻塞加写锁，只锁自己的区间 */
        if (lock_range(fd, F_WRLCK, start, len, 1) == -1) {
            perror("lock_range");
            exit(1);
        }
        printf("[%s] locked [%ld,%ld), writing pass %d\n",
               who, (long)start, (long)(start + len), i + 1);

        /* 在区间内写自己的字符 */
        {
            off_t j;
            for (j = start; j < start + len; j++) {
                if (pwrite(fd, &c, 1, j) == -1) {
                    perror("pwrite");
                    exit(1);
                }
            }
        }
        sleep(1);

        /* 解锁自己的区间 */
        if (lock_range(fd, F_UNLCK, start, len, 1) == -1) {
            perror("unlock");
            exit(1);
        }
        printf("[%s] unlocked\n", who);

        usleep(200000);
    }
}

int main(int argc, char *argv[])
{
    const char *path;
    int fd;
    pid_t pa, pb;
    off_t half = FILE_SIZE / 2;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }
    path = argv[1];

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) { perror("open"); return 1; }

    /* 先把文件填满空格，便于观察 */
    {
        int i;
        for (i = 0; i < FILE_SIZE; i++)
            if (write(fd, " ", 1) == -1) { perror("write"); return 1; }
    }

    printf("=== fcntl 字节范围锁演示 ===\n");
    printf("文件大小 %d 字节，分成两半：\n", FILE_SIZE);
    printf("  进程 A 锁前 %ld 字节\n", (long)half);
    printf("  进程 B 锁后 %ld 字节\n\n", (long)(FILE_SIZE - half));

    pa = fork();
    if (pa == -1) { perror("fork"); return 1; }
    if (pa == 0) {
        /* A 写前半段 */
        writer(fd, "A", 0, half);
        _exit(0);
    }

    pb = fork();
    if (pb == -1) { perror("fork"); return 1; }
    if (pb == 0) {
        /* B 写后半段 */
        writer(fd, "B", half, FILE_SIZE - half);
        _exit(0);
    }

    /* 父进程：顺便演示 F_GETLK 探测 */
    {
        int i;
        for (i = 0; i < 3; i++) {
            sleep(2);
            {
                pid_t owner = probe_range(fd, 0, half);
                printf("[PARENT] probe 前半段 -> %s\n",
                       owner == 0 ? "无锁"
                       : owner == -1 ? "探测失败"
                       : "被进程锁定");
            }
        }
    }

    wait(NULL);
    wait(NULL);

    printf("\n=== 最终文件内容（前 32 字节 + 中部 + 末 32 字节）===\n");
    {
        char buf[64];
        int n;

        memset(buf, 0, sizeof(buf));
        n = (int)pread(fd, buf, 32, 0);
        fwrite(buf, 1, (size_t)n, stdout);
        printf(" ... ");

        memset(buf, 0, sizeof(buf));
        n = (int)pread(fd, buf, 32, half - 16);
        fwrite(buf, 1, (size_t)n, stdout);
        printf(" ... ");

        memset(buf, 0, sizeof(buf));
        n = (int)pread(fd, buf, 32, FILE_SIZE - 32);
        fwrite(buf, 1, (size_t)n, stdout);
        printf("\n");
    }

    close(fd);
    return 0;
}
