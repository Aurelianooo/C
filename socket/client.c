#include <stdio.h>
#include "../wrap/wrap.h"
#include <stddef.h>
#include <sys/un.h>

#define SRV_ADDR "srv.socket"
#define CLI_ADDR "cli.socket"

int main(void)
{
    int cfd, len;
    struct sockaddr_un srvaddr, cliaddr;
    char buf[2048];

    cfd = Socket(AF_UNIX, SOCK_STREAM, 0);

    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sun_family = AF_UNIX;
    strcpy(cliaddr.sun_path, CLI_ADDR);

    len = offsetof(struct sockaddr_un, sun_path) + strlen(cliaddr.sun_path);

    unlink(CLI_ADDR);
    Bind(cfd, (struct sockaddr *)&cliaddr, len);

    bzero(&srvaddr, sizeof(srvaddr));
    srvaddr.sun_family = AF_UNIX;
    strcpy(srvaddr.sun_path, SRV_ADDR);

    len = offsetof(struct sockaddr_un, sun_path) + strlen(srvaddr.sun_path);

    Connect(cfd, (struct sockaddr *)&srvaddr, len);

    while (fgets(buf, sizeof(buf), stdin) != NULL)
    {
        if(!strncmp(buf, "quit", 4))
            break;
        write(cfd, buf, strlen(buf));
        len = read(cfd, buf, sizeof(buf));
        write(STDOUT_FILENO, buf, len);
    }

    Close(cfd);

    return 0;
}
