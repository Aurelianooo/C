#include "../wrap/wrap.h"
#include <time.h>

#define MAX_EVENTS 1024
#define BUFLEN 1024
#define SERV_PROT 8008

void recvdata(int fd, int events, void *arg);
void senddata(int fd, int events, void *arg);

struct myevent_s
{
    int fd;
    int events;
    void *arg;
    void (*callback)(int fd, int events, void *arg);
    int status;
    char buf[BUFLEN];
    int len;
    long last_active;
};

int g_efd;
struct myevent_s g_events[MAX_EVENTS + 1];

void eventset(struct myevent_s *ev, int fd, void (*callback)(int, int, void *), void *arg)
{
    ev->fd = fd;
    ev->events = 0;
    ev->arg = arg;
    ev->callback = callback;
    // memset(ev->buf, 0, sizeof ev->buf);
    // ev->len = 0;
    ev->last_active = time(NULL);
}

void eventadd(int events, struct myevent_s *ev)
{
    struct epoll_event epv;
    epv.events = ev->events = events;
    epv.data.ptr = ev;

    int op;
    if (ev->status == 0)
    {
        op = EPOLL_CTL_ADD;
        ev->status = 1;
    }

    if (epoll_ctl(g_efd, op, ev->fd, &epv) < 0)
        printf("event add failed [fd=%d], pos[%ld]\n", ev->fd, ev - g_events);
    else
        printf("event add OK [fd=%d], events %d, pos[%ld]\n", ev->fd, events, ev - g_events);
}

void eventdel(struct myevent_s *ev)
{
    if (ev->status != 1)
        return;

    struct epoll_event epv;
    epv.data.ptr = NULL;
    ev->status = 0;
    epoll_ctl(g_efd, EPOLL_CTL_DEL, ev->fd, &epv);
}

void acceptconn(int lfd, int events, void *arg)
{
    struct sockaddr_in cin;
    socklen_t len = sizeof cin;
    int cfd, i;

    cfd = accept(lfd, (struct sockaddr *)&cin, &len);
    if (cfd == -1)
    {
        if (errno != EAGAIN && errno != EINTR)
            // TODO
            printf("%s: accept, %s\n", __func__, strerror(errno));
    }

    do
    {
        for (i = 0; i < MAX_EVENTS; i++)
            if (g_events[i].status == 0)
                break;
        if (i == MAX_EVENTS)
        {
            printf("%s: max connect limit[%d]\n", __func__, MAX_EVENTS);
            break;
        }
        int flag = 0;
        if ((flag = fcntl(cfd, F_SETFL, O_NONBLOCK)) < 0)
        {
            printf("%s: fcntl nonblocking failed, %s\n", __func__, strerror(errno));
            break;
        }

        eventset(&g_events[i], cfd, recvdata, &g_events[i]);
        eventadd(EPOLLIN, &g_events[i]);
    } while (0);
    printf("new connect [%s:%d], pos[%d]\n",
           inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), i);
}

void init_lfd()
{
    int lfd = Socket(AF_INET, SOCK_STREAM, 0);
    fcntl(lfd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof sin);
    sin.sin_family = AF_INET;
    sin.sin_port = ntohs(SERV_PROT);
    sin.sin_addr.s_addr = ntohl(INADDR_ANY);

    Bind(lfd, (struct sockaddr *)&sin, sizeof sin);

    Listen(lfd, 128);

    eventset(&g_events[MAX_EVENTS], lfd, acceptconn, &g_events[MAX_EVENTS]);
    eventadd(EPOLLIN, &g_events[MAX_EVENTS]);
}

void recvdata(int fd, int events, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    int len;
    len = recv(fd, ev->buf, sizeof ev->buf, 0);

    eventdel(ev);

    if (len > 0)
    {
        ev->len = len;
        ev->buf[len] = '\0';
        printf("C[%d]:%s\n", fd, ev->buf);

        eventset(ev, fd, senddata, ev);
        eventadd(EPOLLOUT, ev);
    }
    else if (len == 0)
    {
        Close(ev->fd);
        printf("[fd=%d] pos[%ld], closed\n", fd, ev - g_events);
    }
    else
    {
        Close(ev->fd);
        printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
    }
}

void senddata(int fd, int events, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    int len;

    len = send(fd, ev->buf, ev->len, 0);

    eventdel(ev);

    if (len > 0)
    {
        printf("send[fd=%d],[%d]%s\n", fd, len, ev->buf);
        eventset(ev, fd, recvdata, ev);
        eventadd(EPOLLIN, ev);
    }
    else
    {
        Close(ev->fd);
        printf("send[fd=%d] error %s\n", fd, strerror(errno));
    }
}

int main(void)
{
    g_efd = epoll_create(MAX_EVENTS + 1);
    if (g_efd <= 0)
        printf("create g_efd in %s err %s\n", __func__, strerror(errno));

    init_lfd(g_efd);
    struct epoll_event events[MAX_EVENTS + 1];

    int checkpos = 0, i;
    while (1)
    {
        long now = time(NULL);
        for (i = 0; i < 100; i++, checkpos++)
        {
            if (checkpos == MAX_EVENTS)
                checkpos = 0;
            if (g_events[checkpos].status != 1)
                continue;
            long duration = now - g_events[checkpos].last_active;
            if (duration >= 60)
            {
                printf("[fd=%d] timeout\n", g_events[checkpos].fd);
                eventdel(&g_events[checkpos]);
                Close(g_events[checkpos].fd);
            }
        }

        int nready = epoll_wait(g_efd, events, MAX_EVENTS + 1, 1000);
        if (nready < 0)
        {
            printf("epoll wait error, exit\n");
            break;
        }
        for (i = 0; i < nready; i++)
        {
            struct myevent_s *ev = (struct myevent_s *)events[i].data.ptr;
            if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN))
                ev->callback(ev->fd, events[i].events, ev->arg);
            if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT))
                ev->callback(ev->fd, events[i].events, ev->arg);
        }
    }

    return 0;
}
