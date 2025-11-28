#include "../wrap/wrap.h"

#define SRV_PORT 8888
#define BUFLEN 1024

struct s_info
{
    struct sockaddr_in cliaddr;
    int cfd;
};

void *do_work(void *arg)
{
    int i, n;
    struct s_info *ts = (struct s_info *)arg;
    char buf[BUFLEN];
    char str[INET_ADDRSTRLEN];

    while (1)
    {
        n = Read(ts->cfd, buf, BUFLEN);
        if (n == 0)
        {
            printf("Client %d closed...\n", ts->cfd);
            break;
        }
        printf("Received from %s at PORT %d\n",
               inet_ntop(AF_INET, &ts->cliaddr.sin_addr, str, sizeof str),
               ntohs(ts->cliaddr.sin_port));
        for (i = 0; i < n; i++)
            buf[i] = toupper(buf[i]);

        Write(STDOUT_FILENO, buf, n);
        Write(ts->cfd, buf, n);
    }
    Close(ts->cfd);
    pthread_exit(0);
}

int main(void)
{
    int lfd, cfd;
    int i;
    pthread_t tid;
    struct s_info ts[128];

    struct sockaddr_in srvaddr, cliaddr;
    memset(&srvaddr, 0, sizeof(srvaddr));
    socklen_t cliaddr_len;

    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(SRV_PORT);
    srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    Bind(lfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr));

    Listen(lfd, 128);

    printf("Accepting client connect...\n");

    while (1)
    {
        cliaddr_len = sizeof(cliaddr);
        cfd = Accept(lfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
        ts[i].cfd = cfd;
        ts[i].cliaddr = cliaddr;

        pthread_create(&tid, NULL, do_work, (void *)&ts[i]);
        pthread_detach(tid);
        i++;
    }

    return 0;
}
