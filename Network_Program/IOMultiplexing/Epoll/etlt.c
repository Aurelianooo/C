#include "wrap.h"
#include <time.h>

#define MAXLINE 10

int main(int argc, char *argv[])
{
    int i, j, efd;
    int pfd[2];
    pid_t pid;
    char buf[MAXLINE], ch = 'a';
    int trigger = 0;

    if (argc > 1)
    {
        if (strcmp(argv[1], "-e") == 0)
        {
            trigger = 1;
        }
        else if (strcmp(argv[1], "-E") == 0)
        {
            trigger = 2;
        }
        else if (strcmp(argv[1], "-l") != 0)
        {
            fprintf(stderr, "Unknown option: %s\n", argv[1]);
            return 1;
        }
    }

    pipe(pfd);
    pid = fork();

    if (pid == 0)
    { // child write
        close(pfd[0]);
        for (i = 0; i < 10; i++)
        {
            for (j = 0; j < MAXLINE / 2 - 1; j++)
                buf[j] = ch;
            buf[j++] = '\n'; // aaaa\n
            ch++;
            for (; j < MAXLINE - 1; j++)
                buf[j] = ch;
            buf[j] = '\n'; // bbbb\n
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

        if (!trigger)
        { // Level Trigger (default)
            ev.events = EPOLLIN;
        }
        else
        { // Edge Trigger
            if (trigger == 2)
            { // No Block
                int flags = fcntl(pfd[0], F_GETFL);
                fcntl(pfd[0], F_SETFL, flags | O_NONBLOCK);
            }
            ev.events = EPOLLIN | EPOLLET;
        }

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
                if (trigger == 2)
                {
                    while (1)
                    {
                        n = read(pfd[0], buf, MAXLINE / 2);
                        if (n == -1)
                        { // buffer is empty
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                                break;
                            else
                                sys_error("read error");
                        }
                        else if (n == 0)
                        {
                            close(pfd[0]);
                            close(efd);
                            wait(NULL);
                            return 0;
                        }
                        write(STDOUT_FILENO, buf, n);
                    }
                }
                else
                {
                    n = read(pfd[0], buf, MAXLINE / 2);
                    if (n == 0)
                    {
                        close(pfd[0]);
                        close(efd);
                        wait(NULL);
                        return 0;
                    }
                    write(STDOUT_FILENO, buf, n);
                }
            }
        }
    }
    else
    {
        sys_error("fork error");
    }

    return 0;
}
