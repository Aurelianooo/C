#include "wrap.h"

#define SRV_PORT 2345 // 服务器监听端口

int main(void)
{
    int lfd, cfd;
    socklen_t cliaddr_len;
    struct sockaddr_in srvaddr, cliaddr;
    char buf[2048];
    ssize_t size;
    int i;

    // 忽略SIGPIPE信号，防止客户端异常退出导致服务器崩溃
    signal(SIGPIPE, SIG_IGN);

    // 创建TCP套接字（AF_INET=IPv4，SOCK_STREAM=TCP）
    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    // 初始化服务器地址结构
    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(SRV_PORT);          // 端口号转网络字节序
    srvaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 绑定所有本地接口

    // 绑定地址和端口
    Bind(lfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr));

    // 开始监听
    Listen(lfd, 10);
    printf("Waiting for client connections...\n");

    while (1)
    {
        // 接受客户端连接
        cliaddr_len = sizeof(cliaddr);
        cfd = Accept(lfd, (struct sockaddr *)&cliaddr, &cliaddr_len);

        // 打印客户端IP和端口号
        printf("Client connected: %s:%d\n",
               inet_ntoa(cliaddr.sin_addr), // 网络字节序IP转字符串
               ntohs(cliaddr.sin_port));    // 网络字节序端口转主机字节序

        // 处理客户端数据
        while ((size = Read(cfd, buf, sizeof(buf))) > 0)
        {
            // 将所有字母转换为大写
            for (i = 0; i < size; i++)
                buf[i] = toupper(buf[i]);

            Writen(cfd, buf, size);
        }

        // 客户端关闭连接或出错
        if (size == 0)
            printf("Client disconnected: %s:%d\n",
                   inet_ntoa(cliaddr.sin_addr),
                   ntohs(cliaddr.sin_port));
        else
            perror("Read error");

        Close(cfd);
    }

    Close(lfd);
    return 0;
}