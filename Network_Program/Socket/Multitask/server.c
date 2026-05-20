#include "wrap.h"

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
    char str[INET_ADDRSTRLEN];

    signal(SIGPIPE, SIG_IGN);

    struct sockaddr_in srv_addr, clt_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    socklen_t clt_addr_len = sizeof clt_addr;

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SRV_PORT);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // register SIGCHLD signal handling function
    struct sigaction act;
    act.sa_handler = catch_child;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    ret = sigaction(SIGCHLD, &act, NULL);
    if (ret != 0)
        sys_error("sigaction error");

    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Bind(lfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));

    Listen(lfd, 128);

    printf("Waiting for client connections...\n");
    while (1)
    {
        cfd = Accept(lfd, (struct sockaddr *)&clt_addr, &clt_addr_len);
        printf("Client connected: %s:%d\n",
               inet_ntop(AF_INET, &clt_addr.sin_addr, str, sizeof(str)),
               ntohs(clt_addr.sin_port));

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
            Close(cfd);
            continue;
        }
    }

    if (pid == 0)
    { // child process
        printf("Child process %d started\n", getpid());
        while (1)
        {
            ret = Read(cfd, buf, sizeof(buf));
            if (ret <= 0)
            {
                printf("Client disconnected, child process %d exiting\n", getpid());
                Close(cfd);
                exit(EXIT_SUCCESS);
            }
            for (i = 0; i < ret; i++)
                buf[i] = toupper(buf[i]);
            Writen(cfd, buf, ret);
            Write(STDOUT_FILENO, buf, ret);
        }
    }

    return 0;
}
