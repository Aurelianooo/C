/**
* UDP: nc -u 127.0.0.1 9096
* TCP: nc 127.0.0.1 9096
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

// ===================== 核心切换宏 ====================
// 1 = UDP 模式   0 = TCP 模式
#define UDP_MODE 1
// ====================================================

int sockfd = 0; // 监听套接字（全局，信号处理函数使用）

void do_sigio(int sig)
{
    struct sockaddr_in cli_addr;
    int clilen = sizeof(struct sockaddr_in);

#if UDP_MODE
    // ===================== UDP 处理逻辑 =====================
    char buffer[256] = {0};
    // 接收UDP数据
    int len = recvfrom(sockfd, buffer, 256, 0, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
    printf("[UDP] 收到消息: %s\n", buffer);
    // 原样回声发送
    sendto(sockfd, buffer, len, 0, (struct sockaddr *)&cli_addr, clilen);
#else
    // ===================== TCP 处理逻辑 =====================
    int clifd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    char buffer[256] = {0};
    // 读取TCP客户端数据
    int len = read(clifd, buffer, 256);
    printf("[TCP] 收到消息: %s\n", buffer);
    // 原样回声发送
    write(clifd, buffer, len);
    close(clifd); // TCP必须关闭连接套接字
#endif
}

int main(void)
{
#if UDP_MODE
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
#else
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
#endif

    // 注册 SIGIO 信号
    struct sigaction sigio_action;
    sigio_action.sa_flags = 0;
    sigio_action.sa_handler = do_sigio;
    sigaction(SIGIO, &sigio_action, NULL);

    // 绑定地址端口
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9096);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // SIGIO 异步配置
    fcntl(sockfd, F_SETOWN, getpid());
    int flags = fcntl(sockfd, F_GETFL, 0);
    flags |= O_ASYNC | O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);

    bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

#if (!UDP_MODE)
    listen(sockfd, 5);
#endif

    // 主线程休眠，等待信号通知
    while (1)
        sleep(1);

    close(sockfd);
    return 0;
}
