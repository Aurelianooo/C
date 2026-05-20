#include "wrap.h"

#define MAXLINE 1024
#define SERV_PORT 8001
#define OPEN_MAX 1024

int main(void)
{
    int i, j, n, lfd, cfd, sockfd;
    ssize_t nready, efd, ret;
    char buf[MAXLINE], str[INET_ADDRSTRLEN];

    struct sockaddr_in seraddr, cliaddr;
    memset(&seraddr, 0, sizeof seraddr);
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(SERV_PORT);
    seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t clilen = sizeof cliaddr;

    // epoll 事件结构：tep 用于添加/修改事件，ep 数组存储 epoll_wait 返回的就绪事件
    struct epoll_event tmp, ep[OPEN_MAX];
    signal(SIGPIPE, SIG_IGN);

    lfd = Socket(AF_INET, SOCK_STREAM, 0);
    // 设置非阻塞模式
    // 即使 epoll 使用默认的 LT 模式，也推荐所有 fd 都设为非阻塞，避免单个客户端阻塞整个服务器
    int flags = fcntl(lfd, F_GETFL, 0);
    fcntl(lfd, F_SETFL, flags | O_NONBLOCK);
    // 设置端口复用
    flags = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof flags);

    Bind(lfd, (struct sockaddr *)&seraddr, sizeof seraddr);

    Listen(lfd, 128);

    // 创建 epoll 实例
    // efd = epoll_create(OPEN_MAX);
    efd = epoll_create1(0);
    if (efd == -1)
        sys_error("epoll create error");

    // 配置要监听的事件
    tmp.events = EPOLLIN;                           // 事件类型：可读
    tmp.data.fd = lfd;                              // 事件关联的文件描述符
    ret = epoll_ctl(efd, EPOLL_CTL_ADD, lfd, &tmp); // 将监听 fd 挂到 epoll 树上
    if (ret == -1)
        sys_error("epoll ctrl error");

    printf("Waiting for client connections...\n");
    while (1)
    {
        // 等待事件发生, 返回值 nready：本次就绪的事件数量
        nready = epoll_wait(efd, ep, OPEN_MAX, -1);
        if (nready == -1)
        {
            if (errno == EINTR)
                continue;
            else
                sys_error("epoll wait error");
        }
        for (i = 0; i < nready; i++)
        {                           // 遍历所有就绪的事件逐个处理
            sockfd = ep[i].data.fd; // 获取当前事件关联的fd
            if (sockfd == lfd)
            { // 就绪的是监听 fd, 有新客户端连接
                cfd = Accept(lfd, (struct sockaddr *)&cliaddr, &clilen);
                flags = fcntl(cfd, F_GETFL, 0);
                fcntl(cfd, F_SETFL, flags | O_NONBLOCK);
                printf("received from %s at PORT %d\n",
                       inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof str),
                       ntohs(cliaddr.sin_port));

                tmp.events = EPOLLIN;
                tmp.data.fd = cfd;
                ret = epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &tmp);
                if (ret == -1)
                    sys_error("epoll ctrl error");
            }
            else
            { // 就绪的是客户端 fd, 有客户端数据到来
                n = Read(sockfd, buf, MAXLINE);
                if (n <= 0)
                {
                    ret = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
                    if (ret == -1)
                        sys_error("epoll delete error");
                    Close(sockfd);
                }
                else
                {
                    Write(STDOUT_FILENO, buf, n);
                    for (j = 0; j < n; j++)
                        buf[j] = toupper(buf[j]);
                    Writen(sockfd, buf, n);
                }
            }
        }
    }
    Close(lfd);

    return 0;
}
