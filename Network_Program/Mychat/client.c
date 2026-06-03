#include "wrap.h"

#define SERV_ADDR "127.0.0.1"
#define SERV_PORT 8888

int main(int argc, char *argv[])
{
    char buf[BUFSIZ];
    int i, ret;

    int cfd = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    inet_pton(AF_INET, SERV_ADDR, &server_addr.sin_addr.s_addr);

    Connect(cfd, (struct sockaddr *)&server_addr, sizeof server_addr);

    int flag = fcntl(STDIN_FILENO, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(STDIN_FILENO, F_SETFL, flag);

    flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);

    int efd = epoll_create(2);
    struct epoll_event ev, events[2];
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    epoll_ctl(efd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    ev.data.fd = cfd;
    epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev);

    while (1)
    {
        int nready = epoll_wait(efd, events, 2, -1);
        for (i = 0; i < nready; i++)
        {
            if (events[i].data.fd == STDIN_FILENO)
            {
                ret = Read(STDIN_FILENO, buf, sizeof(buf));
                if (ret > 0)
                    Write(cfd, buf, ret);
                if (!strncmp(buf, "quit", 4))
                    goto exit;
            }
            else if (events[i].data.fd == cfd)
            {
                int ret = Read(cfd, buf, sizeof(buf));
                if (ret > 0)
                    Write(STDOUT_FILENO, buf, ret);
                else if (ret == 0)
                {
                    printf("服务器已断开连接\n");
                    goto exit;
                }
            }
        }
    }

exit:
    close(cfd);
    close(efd);
    return 0;
}
