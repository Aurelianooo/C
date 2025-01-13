#include "../wrap/wrap.h"
#include <time.h>

#define MAX_EVENTS 1024
#define BUFLEN 1024
#define SERV_PROT 8888

#define struct_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)
/**Linus edition*
 * container_of - 通过结构体的一个成员获取容器结构体的指针
 * @ptr: 指向成员的指针。
 * @type: 成员所嵌入的容器结构体类型。
 * @member: 结构体中的成员名。
 */
#define container_of(ptr, type, member) ({             \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
})

void recvdata(int fd, void *arg);
void senddata(int fd, void *arg);

struct myevent_s
{
    struct myevent_s *this;
    int fd;
    int events;
    void (*callback)(int fd, void *arg);
    int status;
    char buf[BUFLEN];
    int len;
    long last_active;
};

int g_efd;
struct myevent_s g_events[MAX_EVENTS + 1];

void eventset(struct myevent_s *ev, int fd, int events, void (*callback)(int, void *))
{
    ev->this = ev;
    ev->fd = fd;
    ev->events = events;
    ev->callback = callback;
    ev->last_active = time(NULL);
}

void eventadd(struct myevent_s *ev)
{
    struct epoll_event epv;
    epv.events = ev->events;
    epv.data.ptr = ev;

    if (ev->status == 0)
        ev->status = 1;

    if (epoll_ctl(g_efd, EPOLL_CTL_ADD, ev->fd, &epv) < 0)
        sys_error("epoll add failed");
}

void eventdel(struct myevent_s *ev)
{
    if (ev->status != 1)
        return;
    ev->status = 0;
    if (epoll_ctl(g_efd, EPOLL_CTL_DEL, ev->fd, NULL) < 0)
        sys_error("epoll del failed");
}

void eventmod(struct myevent_s *ev)
{
    struct epoll_event *epv = struct_entry(ev, struct epoll_event, data.ptr);
    // struct epoll_event *epv = container_of((void *)ev, struct epoll_event, data.ptr);
    epv->events = ev->events;
    if (epoll_ctl(g_efd, EPOLL_CTL_MOD, ev->fd, epv) < 0)
        sys_error("epoll mod failed");
}

void acceptconn(int lfd, void *arg)
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
            sys_error("fcntl nonblocking failed");

        eventset(&g_events[i], cfd, EPOLLIN, recvdata);
        eventadd(&g_events[i]);
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

    eventset(&g_events[MAX_EVENTS], lfd, EPOLLIN, acceptconn);
    eventadd(&g_events[MAX_EVENTS]);
}

void recvdata(int fd, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    int len = recv(fd, ev->buf, sizeof ev->buf, 0);

    if (len > 0)
    {
        ev->len = len;
        ev->buf[len] = '\0';
        printf("recv[fd=%d],[len=%d]:%s", fd, ev->len, ev->buf);

        eventset(ev, fd, EPOLLOUT, senddata);
        eventmod(ev);
    }
    else
    {
        eventdel(ev);
        Close(ev->fd);
        if (len == 0)
            printf("[fd=%d] pos[%ld], closed\n", fd, ev - g_events);
        else
            sys_error("recv error");
    }
}

void senddata(int fd, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    int len = send(fd, ev->buf, ev->len, 0);

    if (len > 0)
    {
        printf("send[fd=%d],[len=%d]:%s", fd, len, ev->buf);
        eventset(ev, fd, EPOLLIN, recvdata);
        eventmod(ev);
    }
    else
    {
        eventdel(ev);
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

    int i;
    // int checkpos = 0, i;
    while (1)
    {
        // long now = time(NULL);
        // for (i = 0; i < 100; i++, checkpos++)
        // {
        //     if (checkpos == MAX_EVENTS)
        //         checkpos = 0;
        //     if (g_events[checkpos].status != 1)
        //         continue;
        //     long duration = now - g_events[checkpos].last_active;
        //     if (duration >= 60)
        //     {
        //         printf("[fd=%d] timeout\n", g_events[checkpos].fd);
        //         eventdel(&g_events[checkpos]);
        //         Close(g_events[checkpos].fd);
        //     }
        // }

        int nready = epoll_wait(g_efd, events, MAX_EVENTS + 1, 1000);
        if (nready == -1)
            sys_error("epoll wait error");
        for (i = 0; i < nready; i++)
        {
            struct myevent_s *ev = (struct myevent_s *)events[i].data.ptr;
            ev->callback(ev->fd, ev->this);
        }
    }

    return 0;
}
