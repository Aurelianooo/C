#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

void sys_error(const char *str)
{
    perror(str);
    exit(1);
}

int main(int argc, char *argv[])
{
    int i;
    char buf[BUFSIZ];
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(7777);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr.s_addr);

    if (connect(cfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        sys_error("connect error");

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
                int ret = read(STDIN_FILENO, buf, sizeof(buf));
                if (ret > 0)
                    write(cfd, buf, ret);
                if (!strncmp(buf, "quit", 4))
                    goto exit;
            }
            else if (events[i].data.fd == cfd)
            {
                int ret = read(cfd, buf, sizeof(buf));
                if (ret > 0)
                    write(STDOUT_FILENO, buf, ret);
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
