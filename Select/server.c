#include <sys/select.h>
#include "../wrap/wrap.h"

#define SRV_PORT 6666

int main(void)
{
    int i, j, n, nready, maxi;
    int client[FD_SETSIZE];
    int maxfd = 0;
    int lfd, cfd, sockfd;
    char buf[BUFSIZ], str[INET_ADDRSTRLEN];

    struct sockaddr_in seraddr, cliaddr;
    memset(&seraddr, 0, sizeof(seraddr));
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(SRV_PORT);
    seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t cliaddr_len;

    lfd = Socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    Bind(lfd, (struct sockaddr *)&seraddr, sizeof seraddr);

    Listen(lfd, 128);

    maxfd = lfd;
    maxi = -1;
    memset(client, -1, sizeof client);
    fd_set rset, allset;
    FD_ZERO(&allset);
    FD_SET(lfd, &allset);

    while (1)
    {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready < 0)
            sys_error("select error");
        if (FD_ISSET(lfd, &rset))
        {
            cliaddr_len = sizeof(cliaddr);
            cfd = Accept(lfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
            printf("received from %s at PORT %d\n",
                   inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof str),
                   ntohs(cliaddr.sin_port));
            FD_SET(cfd, &allset);

            for (i = 0; i < FD_SETSIZE; i++)
            {
                if (client[i] < 0)
                {
                    client[i] = cfd;
                    break;
                }
            }
            if (i == FD_SETSIZE)
                sys_error("too many clients");
            maxi = maxi > i ? maxi : i;
            maxfd = maxfd > cfd ? maxfd : cfd;
            if (--nready == 0)
                continue;
        }

        // for (i = lfd + 1; i <= maxfd; i++)
        // {
        //     if (FD_ISSET(i, &rset))
        //     {
        //         n = Read(i, buf, sizeof buf);
        //         if (n == 0)
        //         {
        //             Close(i);
        //             FD_CLR(i, &allset);
        //         }
        //         else if (n > 0)
        //         {
        //             Write(STDOUT_FILENO, buf, n);
        //             for (j = 0; j < n; j++)
        //                 buf[j] = toupper(buf[j]);
        //             Write(i, buf, n);
        //         }
        //     }
        // }

        for (i = 0; i <= maxi; i++)
        {
            sockfd = client[i];
            if (sockfd < 0)
                continue;
            if (FD_ISSET(sockfd, &rset))
            {
                n = Read(sockfd, buf, sizeof buf);
                if (n == 0)
                {
                    Close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                }
                else if (n > 0)
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
