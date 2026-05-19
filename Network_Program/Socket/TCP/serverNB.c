#include "wrap.h"

#define SRV_PORT 2345
#define MAX_CLIENTS 1024 // 最大支持的客户端数量

int main(void)
{
    int lfd, cfd;
    socklen_t cliaddr_len;
    struct sockaddr_in srvaddr, cliaddr;
    char buf[2048];
    ssize_t size;
    int i, j;

    // 客户端fd数组，保存所有已连接的客户端
    int clients[MAX_CLIENTS];
    memset(clients, -1, sizeof(clients));
    signal(SIGPIPE, SIG_IGN);

    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    // 将监听套接字设置为非阻塞模式
    int flags = fcntl(lfd, F_GETFL, 0);
    fcntl(lfd, F_SETFL, flags | O_NONBLOCK);

    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(SRV_PORT);
    srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    Bind(lfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr));
    Listen(lfd, 10);
    printf("Waiting for client connections...\n");

    while (1)
    {
        // 非阻塞接受新连接
        cliaddr_len = sizeof(cliaddr);
        cfd = accept(lfd, (struct sockaddr *)&cliaddr, &cliaddr_len);

        if (cfd > 0)
        {
            printf("Client connected: %s:%d\n",
                   inet_ntoa(cliaddr.sin_addr),
                   ntohs(cliaddr.sin_port));

            // 将新客户端套接字也设置为非阻塞
            flags = fcntl(cfd, F_GETFL, 0);
            fcntl(cfd, F_SETFL, flags | O_NONBLOCK);

            // 保存客户端fd
            for (i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i] == -1)
                {
                    clients[i] = cfd;
                    break;
                }
            }

            if (i == MAX_CLIENTS)
            {
                printf("Too many clients, closing connection\n");
                Close(cfd);
            }
        }
        else if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("accept error");
        }

        // 遍历所有客户端，非阻塞处理数据
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i] == -1)
                continue;

            cfd = clients[i];
            size = Read(cfd, buf, sizeof(buf));
            if (size > 0)
            {
                for (j = 0; j < size; j++)
                    buf[j] = toupper(buf[j]);
                ssize_t written = Writen(cfd, buf, size);
                if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    perror("write error");
                    Close(cfd);
                    clients[i] = -1;
                }
            }
            else if (size == 0)
            {
                printf("Client disconnected\n");
                Close(cfd);
                clients[i] = -1;
            }
            else if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("read error");
                Close(cfd);
                clients[i] = -1;
            }
        }
    }

    Close(lfd);
    return 0;
}