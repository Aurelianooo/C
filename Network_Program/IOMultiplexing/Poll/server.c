#include <poll.h>
#include "../wrap/wrap.h"

#define MAXLINE 1024
#define SERV_PORT 8000
#define OPEN_MAX 1024

int main(void)
{
    int i, j, maxi, lfd, cfd, sockfd;
    int nready;
    ssize_t n;
    char buf[MAXLINE], str[INET_ADDRSTRLEN];

    struct sockaddr_in srvaddr, cltaddr;
    memset(&srvaddr, 0, sizeof srvaddr);
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(SERV_PORT);
    srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t cltaddrlen = sizeof cltaddr;

    struct pollfd client[OPEN_MAX];

    lfd = Socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    Bind(lfd, (struct sockaddr *)&srvaddr, sizeof srvaddr);

    Listen(lfd, 128);

    client[0].fd = lfd;
    client[0].events = POLLIN;

    for (i = 1; i < OPEN_MAX; i++)
        client[i].fd = -1;
    maxi = 0;

    while (1)
    {
        nready = poll(client, maxi + 1, -1);
        if (client[0].revents & POLLIN)
        {
            cfd = Accept(lfd, (struct sockaddr *)&cltaddr, &cltaddrlen);
            printf("received from %s at PORT %d\n",
                   inet_ntop(AF_INET, &cltaddr.sin_addr, str, sizeof str),
                   ntohs(cltaddr.sin_port));

            for (i = 1; i < OPEN_MAX; i++)
                if (client[i].fd < 0)
                {
                    client[i].fd = cfd;
                    break;
                }
            if (i == OPEN_MAX)
                sys_error("Too many clients");
            client[i].events = POLLIN;
            maxi = maxi > i ? maxi : i;
            if (--nready == 0)
                continue;
        }

        for (i = 1; i <= maxi; i++)
        {
            if ((sockfd = client[i].fd) < 0)
                continue;
            if (client[i].revents & POLLIN)
            {
                if ((n = Read(sockfd, buf, MAXLINE)) < 0)
                {
                    if (errno == ECONNRESET)
                    { // receive RST signal
                        printf("client[%d] reset connection\n", i);
                        Close(sockfd);
                        client[i].fd = -1;
                    }
                }
                else if (n == 0)
                {
                    printf("client[%d] closed connection\n", i);
                    Close(sockfd);
                    client[i].fd = -1;
                }
                else
                {
                    Write(STDOUT_FILENO, buf, n);
                    for (j = 0; j < n; j++)
                        buf[j] = toupper(buf[j]);
                    Write(sockfd, buf, n);
                }
                if (--nready == 0)
                    break;
            }
        }
    }
    Close(lfd);

    return 0;
}
