#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <assert.h>

#ifndef SO_REUSEPORT
#define SO_REUSEPORT 15
#endif

#define MAXBUF 10240
#define MAXEPOLLSIZE 10000
#define WORKER_NUM 4 // 定义4个 Worker 线程

volatile int total_cnt = 0;

// Worker 线程上下文
typedef struct
{
    pthread_t tid;
    int epoll_fd; // 每个 Worker 独立的 epoll
    int pipe_fd;  // 管道读端：用于接收主线程发过来的新 socket
    int id;       // 线程编号
} worker_t;

worker_t workers[WORKER_NUM];

// 设置非阻塞的辅助函数
int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int read_data(int sd)
{
    char recvbuf[MAXBUF + 1];
    int ret;
    struct sockaddr_in client_addr;
    socklen_t cli_len = sizeof(client_addr);

    bzero(recvbuf, MAXBUF + 1);
    ret = recvfrom(sd, recvbuf, MAXBUF, 0, (struct sockaddr *)&client_addr, &cli_len);
    if (ret <= 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            printf("read err:%s  %d\n", strerror(errno), ret);
        }
    }
    return ret;
}

// 专注创建连接
int udp_accept(int sd, struct sockaddr_in my_addr)
{
    int new_sd = -1;
    int ret = 0;
    int reuse = 1;
    char dummy_buf[MAXBUF]; // 用来把第一个包读掉，否则它会一直触发 epoll
    struct sockaddr_in peer_addr;
    socklen_t cli_len = sizeof(peer_addr);

    // 接收数据（必须读掉，否则 epoll 会一直通知）
    ret = recvfrom(sd, dummy_buf, 16, 0, (struct sockaddr *)&peer_addr, &cli_len);
    if (ret < 0)
    {
        return -1;
    }

    // 创建新 Socket
    if ((new_sd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("child socket");
        return -1;
    }
    set_nonblocking(new_sd);

    // 设置端口复用
    ret = setsockopt(new_sd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (ret)
    {
        close(new_sd);
        return -1;
    }
    ret = setsockopt(new_sd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    if (ret)
    {
        close(new_sd);
        return -1;
    }

    ret = bind(new_sd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    if (ret)
    {
        perror("child bind");
        close(new_sd);
        return -1;
    }

    // Connect 到客户端
    if (connect(new_sd, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("child connect");
        close(new_sd);
        return -1;
    }

    __sync_add_and_fetch(&total_cnt, 1); // 原子操作累加计数（这里非必要）
    return new_sd;
}

// Worker 线程的主循环
void *worker_thread(void *arg)
{
    worker_t *worker = (worker_t *)arg;
    struct epoll_event events[MAXEPOLLSIZE];

    printf("Worker %d started.\n", worker->id);
    while (1)
    {
        int nfds = epoll_wait(worker->epoll_fd, events, MAXEPOLLSIZE, -1);
        if (nfds == -1)
        {
            if (errno == EINTR)
                continue;
            perror("epoll_wait worker");
            break;
        }

        for (int n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == worker->pipe_fd)
            {
                // 情况1：管道有数据，说明主线程发来了一个新的 socket fd
                int new_client_fd;
                // 从管道读取 fd (注意：这里是读一个整数，不是读普通数据)
                int r = read(worker->pipe_fd, &new_client_fd, sizeof(new_client_fd));
                if (r == sizeof(new_client_fd))
                { // 将新 client 加入到自己的 epoll 中
                    struct epoll_event ev;
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = new_client_fd;
                    if (epoll_ctl(worker->epoll_fd, EPOLL_CTL_ADD, new_client_fd, &ev) < 0)
                    {
                        close(new_client_fd);
                    }
                }
            }
            else
            {
                // 情况2：普通 client socket 有数据
                read_data(events[n].data.fd);
                // 注意：这里没有 close，实际生产中需要处理超时关闭逻辑
            }
        }
    }
}

int main(int argc, char **argv)
{
    int listener, kdpfd, nfds, n;
    struct sockaddr_in my_addr;
    unsigned int port = 1234;
    struct epoll_event ev;
    struct epoll_event events[MAXEPOLLSIZE];
    int opt = 1;
    int ret = 0;
    int rr_index = 0; // Round-Robin 索引，用于负载均衡

    // 初始化 Worker 线程和管道
    for (int i = 0; i < WORKER_NUM; i++)
    {
        int pipefd[2];
        if (pipe(pipefd) == -1)
        {
            perror("pipe");
            exit(1);
        }

        workers[i].id = i;
        workers[i].pipe_fd = pipefd[0]; // 读端留给 worker
        workers[i].epoll_fd = epoll_create(MAXEPOLLSIZE);

        // 把管道读端加入 worker 自己的 epoll
        struct epoll_event pipe_ev;
        pipe_ev.events = EPOLLIN;
        pipe_ev.data.fd = workers[i].pipe_fd;
        epoll_ctl(workers[i].epoll_fd, EPOLL_CTL_ADD, workers[i].pipe_fd, &pipe_ev);

        // 创建线程
        pthread_create(&workers[i].tid, NULL, worker_thread, &workers[i]);
    }

    // 创建 Listener
    if ((listener = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    set_nonblocking(listener);

    ret = setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret)
    {
        perror("setsockopt REUSEADDR");
        exit(1);
    }

    ret = setsockopt(listener, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    if (ret)
    {
        perror("setsockopt REUSEPORT");
        exit(1);
    }

    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = PF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listener, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    // 主线程的 Epoll 只用来监听 listener
    kdpfd = epoll_create(MAXEPOLLSIZE);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listener;
    if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, listener, &ev) < 0)
    {
        fprintf(stderr, "epoll set insertion error: fd=%dn", listener);
        return -1;
    }

    printf("Server running... Waiting for clients.\n");
    // 主线程循环：只负责 Accept 和分发
    while (1)
    {
        nfds = epoll_wait(kdpfd, events, MAXEPOLLSIZE, -1);
        if (nfds == -1)
        {
            if (errno == EINTR)
                continue;
            perror("epoll_wait main");
            break;
        }

        for (n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == listener)
            {
                while (1)
                {
                    int new_sd = udp_accept(listener, my_addr);
                    if (new_sd == -1)
                    {
                        break;
                    }
                    // 关键逻辑：通过管道将 new_sd 分发给 Worker
                    // 选择下一个 Worker (Round-Robin)
                    worker_t *target_worker = &workers[rr_index % WORKER_NUM];
                    rr_index++;
                    // 将 new_sd 写入管道
                    // 注意：我们传递的是文件描述符的整数值
                    write(target_worker->pipe_fd, &new_sd, sizeof(new_sd));
                }
                printf("Current connection count: %d\n", total_cnt);
                fflush(stdout);
            }
        }
    }

    close(listener);
    return 0;
}
