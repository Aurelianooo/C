#include "../../wrap/wrap.h"
#include <time.h>

#define MAXLINE 10

int main(void)
{
    int i, j, efd;
    int pfd[2];
    pid_t pid;
    char buf[MAXLINE], ch = 'a';

    pipe(pfd);
    pid = fork();

    if (pid == 0)
    { // child write
        close(pfd[0]);
        for (i = 0; i < 10; i++)
        {
            // aaaa\n
            for (j = 0; j < MAXLINE / 2 - 1; j++)
                buf[j] = ch;
            buf[j++] = '\n';
            ch++;
            // bbbb\n
            for (; j < MAXLINE - 1; j++)
                buf[j] = ch;
            buf[j] = '\n';
            ch++;
            write(pfd[1], buf, sizeof buf);
            sleep(5);
        }
        close(pfd[1]);
    }
    else if (pid > 0)
    { // father read
        close(pfd[1]);

        struct epoll_event ev;
        struct epoll_event revent[10];
        int n;
        efd = epoll_create(10);

        ev.events = EPOLLIN; // Level Trigger (default)
        // ev.events = EPOLLIN | EPOLLET; // Edge Trigger
        ev.data.fd = pfd[0];
        epoll_ctl(efd, EPOLL_CTL_ADD, pfd[0], &ev);

        struct tm *t;
        time_t tt;
        while (1)
        {
            epoll_wait(efd, revent, 10, -1);
            time(&tt);
            t = localtime(&tt);
            printf("%02d:%02d:%02d\n", t->tm_hour, t->tm_min, t->tm_sec);
            if (revent[0].data.fd == pfd[0])
            {
                n = read(pfd[0], buf, MAXLINE / 2);
                write(STDOUT_FILENO, buf, n);
            }
        }
        close(pfd[0]);
        close(efd);
    }
    else
    {
        sys_error("fork error");
    }

    return 0;
}
