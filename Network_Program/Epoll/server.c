#include "../wrap/wrap.h"

#define MAXLINE 1024
#define SERV_PORT 8001
#define OPEN_MAX 1024

int main(void)
{
    int i, n, lfd, cfd, sockfd;
    ssize_t nready, efd, ret;
    char buf[MAXLINE], str[INET_ADDRSTRLEN];

    struct sockaddr_in seraddr, cliaddr;
    memset(&seraddr, 0, sizeof seraddr);
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(SERV_PORT);
    seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t clilen = sizeof cliaddr;

    struct epoll_event tep, ep[OPEN_MAX];

    lfd = Socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    Bind(lfd, (struct sockaddr *)&seraddr, sizeof seraddr);

    Listen(lfd, 128);

    efd = epoll_create(OPEN_MAX);
    if (efd == -1)
        sys_error("epoll create error");

    tep.events = EPOLLIN;
    tep.data.fd = lfd;
    ret = epoll_ctl(efd, EPOLL_CTL_ADD, lfd, &tep);
    if (ret == -1)
        sys_error("epoll ctrl error");

    while (1)
    {
        nready = epoll_wait(efd, ep, OPEN_MAX, -1);
        if (nready == -1)
            sys_error("epoll wait error");
        for (i = 0; i < nready; i++)
        {
            sockfd = ep[i].data.fd;
            if (sockfd == lfd)
            {
                cfd = Accept(lfd, (struct sockaddr *)&cliaddr, &clilen);
                printf("received from %s at PORT %d\n",
                       inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof str),
                       ntohs(cliaddr.sin_port));

                tep.events = EPOLLIN;
                tep.data.fd = cfd;
                ret = epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &tep);
                if (ret == -1)
                    sys_error("epoll ctrl error");
            }
            else
            {
                n=Read(sockfd, buf, MAXLINE);
                if(n<=0){
                    ret=epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
                    if(ret==-1)
                        sys_error("epoll delete error");
                    Close(sockfd);
                }else{
                    Write(STDOUT_FILENO, buf, n);
                    for(i=0;i<n;i++)
                        buf[i]=toupper(buf[i]);
                    Write(sockfd, buf, n);
                }
            }
        }
    }
    Close(lfd);

    return 0;
}
