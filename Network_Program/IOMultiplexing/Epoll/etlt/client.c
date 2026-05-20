#include "../../wrap/wrap.h"

#define MAXLINE 10
#define SERV_PORT 8001

int main(void)
{

    int sockfd, i, j;
    char buf[MAXLINE], ch = 'a';

    struct sockaddr_in seraddr;
    memset(&seraddr, 0, sizeof seraddr);
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, "127.0.0.1", &seraddr.sin_addr);

    sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    Connect(sockfd, (struct sockaddr *)&seraddr, sizeof seraddr);

    for (i = 0; i < 3; i++)
    {
        // aaaa\n
        for (j = 0; j < MAXLINE / 2 - 1; j++)
            buf[j] = ch;
        buf[j++] = '\n';
        ch++;
        // bbbb\n
        for (; j < MAXLINE - 1; j++)
            buf[j] = ch;
        buf[j] = '\n';
        ch++;
        Write(sockfd, buf, sizeof buf);
        sleep(3);
    }
    Close(sockfd);

    return 0;
}
