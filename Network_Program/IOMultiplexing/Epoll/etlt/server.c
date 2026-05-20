#include "../../wrap/wrap.h"
#include <time.h>

#define MAXLINE 6
#define SERV_PORT 8001

int main(void)
{
    int lfd, cfd, efd, n;
    char buf[MAXLINE], str[INET_ADDRSTRLEN];
    struct tm *t;
    time_t tt;

    struct sockaddr_in seraddr, cliaddr;
    memset(&seraddr, 0, sizeof seraddr);
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(SERV_PORT);
    seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t clilen = sizeof cliaddr;

    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    Bind(lfd, (struct sockaddr *)&seraddr, sizeof seraddr);

    Listen(lfd, 128);

    struct epoll_event ev;
    struct epoll_event revent[10];

    efd = epoll_create(10);
    ev.events = EPOLLIN | EPOLLET;
    // ev.events=EPOLLIN;
    printf("Accepting connections...\n");
    cfd = Accept(lfd, (struct sockaddr *)&cliaddr, &clilen);
    printf("received from %s at PORT %d\n",
           inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof str),
           ntohs(cliaddr.sin_port));

    // int flag = fcntl(cfd, F_GETFL); // set nonblock
    // flag |= O_NONBLOCK;
    // fcntl(cfd, F_SETFL, flag);

    ev.data.fd = cfd;
    epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev);

    while (1)
    {
        epoll_wait(efd, revent, 10, -1);
        time(&tt);
        t = localtime(&tt);
        printf("%02d:%02d:%02d\n", t->tm_hour, t->tm_min, t->tm_sec);
        if (revent[0].data.fd == cfd)
        {
            while ((n = Read(cfd, buf, MAXLINE / 2)) > 0)
                Write(STDOUT_FILENO, buf, n);
            if (n == 0)
                break;
        }
    }

    epoll_ctl(efd, EPOLL_CTL_DEL, cfd, NULL);
    Close(efd);
    Close(cfd);
    Close(lfd);

    return 0;
}
