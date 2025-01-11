
#include "../wrap/wrap.h"

int main(int argc, char *argv[])
{
    char buf[BUFSIZ];
    int ret;

    int cfd = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8001);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr.s_addr);

    Connect(cfd, (struct sockaddr *)&server_addr, sizeof server_addr);

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
