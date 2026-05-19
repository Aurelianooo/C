#include "wrap.h"

#define SRV_IP "127.0.0.1" // 服务器IP地址
#define SRV_PORT 2345      // 服务器端口号

int main(void)
{
    int cfd;
    struct sockaddr_in srvaddr;
    char buf[2048];
    ssize_t len;

    // 创建TCP套接字
    cfd = Socket(AF_INET, SOCK_STREAM, 0);

    // 初始化服务器地址结构
    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(SRV_PORT);
    inet_pton(AF_INET, SRV_IP, &srvaddr.sin_addr);

    // 连接服务器
    Connect(cfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr));

    while (fgets(buf, sizeof(buf), stdin) != NULL)
    {
        if (strncmp(buf, "quit", 4) == 0)
            break;
        Writen(cfd, buf, strlen(buf));
        len = Read(cfd, buf, sizeof(buf));
        if (len <= 0)
        {
            printf("Server closed the connection\n");
            break;
        }
        Write(STDOUT_FILENO, buf, len);
    }

    Close(cfd);
    printf("Connection closed\n");

    return 0;
}