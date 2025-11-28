#include <stdio.h>
#include "../wrap/wrap.h"
#include <stddef.h>
#include <sys/un.h>

#define SRV_ADDR "srv.socket"

int main(void)
{
    int lfd, cfd, len, size, i;
    struct sockaddr_un srvaddr, cliaddr;
    char buf[2048];

    lfd = Socket(AF_UNIX, SOCK_STREAM, 0);

    bzero(&srvaddr, sizeof(srvaddr));
    srvaddr.sun_family = AF_UNIX;
    strcpy(srvaddr.sun_path, SRV_ADDR);

    len = offsetof(struct sockaddr_un, sun_path) + strlen(srvaddr.sun_path);

    unlink(SRV_ADDR);
    Bind(lfd, (struct sockaddr *)&srvaddr, len);

    Listen(lfd, 10);
    printf("Accept ...\n");

    while (1)
    {
        len = sizeof(cliaddr);
        cfd = Accept(lfd, (struct sockaddr *)&cliaddr, (socklen_t *)&len);
        len -= offsetof(struct sockaddr_un, sun_path);
        cliaddr.sun_path[len] = '\0';
        printf("client bind filename %s\n", cliaddr.sun_path);

        while ((size = read(cfd, buf, sizeof(buf))) > 0)
        {
            for (i = 0; i < size; i++)
                buf[i] = toupper(buf[i]);
            write(cfd, buf, size);
        }
        Close(cfd);
    }
    Close(lfd);
    return 0;
}
