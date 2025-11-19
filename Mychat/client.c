#include "../wrap/wrap.h"

#define SERV_ADDR "127.0.0.1"
#define SERV_PORT 8888

int main(int argc, char *argv[])
{
    char buf[BUFSIZ];
    int ret;

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

    while (1)
    {
        ret = Read(STDIN_FILENO, buf, sizeof buf);
        if (!strncmp(buf, "quit", 4))
            break;
        Write(cfd, buf, ret);
        ret = Read(cfd, buf, sizeof buf);
        Write(STDOUT_FILENO, buf, ret);
    }

    close(cfd);
    return 0;
}
