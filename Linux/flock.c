/* flock() 演示：阻塞/非阻塞 + 共享/排他锁

   编译: gcc -std=c89 -Wall -pedantic flock.c -o flock
   运行: ./flock <共享文件路径> [--nb] [--shared]

   示例:
     ./flock tmp                 排他锁模式（写）
     ./flock tmp --nb            非阻塞排他锁模式
     ./flock tmp --shared        共享锁模式（读）

   注意：fork 后父子进程必须各自独立 open 文件，不能共用 fork 前的 fd。
   因为 fork 后父子共享同一个内核 struct file（file description），
   flock 锁是挂在 file description 上的，共用一个不会产生冲突。
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define LINES 5

/* 写入：始终用排他锁 */
static void write_records(const char *path, const char *who,
                          int nonblock, int delay_first)
{
    int fd;
    int base_op;
    int i;
    int retries;

    fd = open(path, O_WRONLY | O_APPEND);
    if (fd == -1) { perror("open"); exit(1); }

    base_op = LOCK_EX;
    if (nonblock)
        base_op |= LOCK_NB;

    for (i = 0; i < LINES; i++) {

        if (delay_first && i == 0) {
            printf("[%s] delaying 3s before first lock attempt...\n", who);
            sleep(3);
        }

        retries = 0;
        while (flock(fd, base_op) == -1) {
            if (errno == EWOULDBLOCK) {
                retries++;
                printf("[%s] lock busy (attempt %d), "
                       "waiting 1s...\n", who, retries);
                sleep(1);
                if (retries >= 3) {
                    printf("[%s] backing off 3s...\n", who);
                    sleep(3);
                }
            } else {
                perror("flock");
                exit(1);
            }
        }

        if (retries > 0)
            printf("[%s] acquired lock after %d retries, "
                   "writing line %d\n", who, retries, i + 1);
        else
            printf("[%s] got lock, writing line %d\n", who, i + 1);

        {
            char buf[64];
            int len;
            len = sprintf(buf, "[%s] line %d\n", who, i + 1);
            if (write(fd, buf, (size_t)len) == -1) {
                perror("write");
                exit(1);
            }
            sleep(2);
        }

        if (flock(fd, LOCK_UN) == -1) {
            perror("flock LOCK_UN");
            exit(1);
        }

        sleep(1);
    }

    close(fd);
}

/* 读取：用共享锁 */
static void read_records(const char *path, const char *who, int nonblock)
{
    int fd;
    int base_op;
    int i;
    int retries;

    fd = open(path, O_RDONLY);
    if (fd == -1) { perror("open"); exit(1); }

    base_op = LOCK_SH;
    if (nonblock)
        base_op |= LOCK_NB;

    for (i = 0; i < LINES; i++) {
        retries = 0;
        while (flock(fd, base_op) == -1) {
            if (errno == EWOULDBLOCK) {
                retries++;
                printf("[%s READ] lock busy (attempt %d), "
                       "waiting 1s...\n", who, retries);
                sleep(1);
            } else {
                perror("flock");
                exit(1);
            }
        }

        if (retries > 0)
            printf("[%s READ] acquired lock after %d retries\n",
                   who, retries);

        {
            char buf[256];
            ssize_t n;
            n = read(fd, buf, sizeof(buf) - 1);
            buf[n > 0 ? n : 0] = '\0';
            printf("[%s READ pass %d] read %zd bytes\n",
                   who, i + 1, n);
        }

        if (flock(fd, LOCK_UN) == -1) {
            perror("flock LOCK_UN");
            exit(1);
        }

        sleep(1);
    }

    close(fd);
}

int main(int argc, char *argv[])
{
    const char *path;
    int nonblock;
    int shared;
    pid_t pid;
    int i;

    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Usage: %s <file> [--nb] [--shared]\n", argv[0]);
        return 1;
    }
    path     = argv[1];
    nonblock = 0;
    shared   = 0;

    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--nb") == 0)
            nonblock = 1;
        else if (strcmp(argv[i], "--shared") == 0)
            shared = 1;
    }

    /* 创建文件并写入初始行，写完后 close，让各进程自己 open */
    {
        int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) { perror("open"); return 1; }
        if (write(fd, "# shared file\n", 14) == -1) {
            perror("write");
            return 1;
        }
        close(fd);
    }

    if (shared) {
        printf("=== LOCK_SH + LOCK_EX demo ===\n\n");
        printf("Parent: LOCK_SH reader, Child: LOCK_EX writer\n");
        printf("Reader uses shared lock (concurrent reads OK)\n");
        printf("Writer uses exclusive lock (blocks readers)\n\n");
    } else {
        printf("=== LOCK_EX only (both write) ===\n");
        if (nonblock)
            printf("Mode: non-blocking\n\n");
        else
            printf("Mode: blocking\n\n");
    }

    pid = fork();
    if (pid == -1) { perror("fork"); return 1; }

    if (pid == 0) {
        write_records(path, "CHILD", nonblock, 0);
    } else {
        if (shared) {
            read_records(path, "PARENT", nonblock);
        } else {
            write_records(path, "PARENT", nonblock, 1);
        }
    }

    if (pid > 0) {
        wait(NULL);
        printf("\n=== 最终文件内容 ===\n");
        {
            FILE *f = fopen(path, "r");
            if (f) {
                char line[128];
                while (fgets(line, sizeof(line), f))
                    fputs(line, stdout);
                fclose(f);
            }
        }
    }

    return 0;
}
