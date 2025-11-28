#include "../wrap/wrap.h"

#define SRV_PORT 9999

void catch_child(int signum)
{
    while (waitpid(0, NULL, WNOHANG) > 0)
        ;
}

int main(void)
{
    int lfd, cfd;
    pid_t pid;
    int ret, i;
    char buf[BUFSIZ];

    struct sockaddr_in srv_addr, clt_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    socklen_t clt_addr_len = sizeof clt_addr;

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SRV_PORT);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct sigaction act;
    act.sa_handler = catch_child;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    Bind(lfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));

    Listen(lfd, 128);

    while (1)
    {

        cfd = Accept(lfd, (struct sockaddr *)&clt_addr, &clt_addr_len);

        pid = fork();
        if (pid < 0)
            sys_error("fork error");
        else if (pid == 0)
        {               // child process
            Close(lfd); // no need to listen and accept
            break;      // work outside while
        }
        else
        { // main process
            ret = sigaction(SIGCHLD, &act, NULL);
            if (ret != 0)
                sys_error("sigaction error");
            close(cfd);
            continue;
        }
    }

    if (pid == 0)
    { // child process
        while (1)
        {
            ret = Read(cfd, buf, sizeof(buf));
            if (ret == 0)
            {
                close(cfd);
                exit(1);
            }
            for (i = 0; i < ret; i++)
                buf[i] = toupper(buf[i]);
            Write(cfd, buf, ret);
            write(STDOUT_FILENO, buf, ret);
        }
    }

    return 0;
}
