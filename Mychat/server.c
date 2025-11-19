#include "chat.h"

static int g_efd;
static int uid;
static int online_num;
static struct myevent_s g_events[MAX_EVENTS + 1];
static const char *user_path = "./user_msg";
static struct TireTree *user_tree;
static char message[] = "--------------------- Connect Server ---------------------[OK]\n"
                        "                   Online ChatRoom BETA\n"
                        "           (1) Anonymous  (2) Login  (3) Sign up\n"
                        ">$ ";

void event_set(struct myevent_s *ev, int fd, int events, void (*callback)(int, void *))
{
    ev->fd = fd;
    ev->events = events;
    ev->callback = callback;
    ev->last_active = time(NULL);
}

void event_add(struct myevent_s *ev)
{
    struct epoll_event epv;
    epv.events = ev->events;
    epv.data.ptr = (void *)ev;
    if (ev->status == OFFLINE)
        ev->status = ONLINE;

    if (epoll_ctl(g_efd, EPOLL_CTL_ADD, ev->fd, &epv) < 0)
        sys_error("epoll add failed");
}

void event_del(struct myevent_s *ev)
{
    if (epoll_ctl(g_efd, EPOLL_CTL_DEL, ev->fd, NULL) < 0)
        sys_error("epoll del failed");
}

void event_mod(struct myevent_s *ev)
{
    struct epoll_event epv;
    epv.events = ev->events;
    epv.data.ptr = (void *)ev;

    if (epoll_ctl(g_efd, EPOLL_CTL_MOD, ev->fd, &epv) < 0)
        sys_error("epoll mod failed");
}

void accept_conn(int lfd, void *arg)
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
            if (g_events[i].status == OFFLINE)
                break;
        if (i == MAX_EVENTS)
        {
            printf("%s: max connect limit[%d]\n", __func__, MAX_EVENTS);
            break;
        }
        int flag = fcntl(cfd, F_GETFL);
        flag |= O_NONBLOCK;
        fcntl(cfd, F_SETFL, flag);

        send(cfd, message, sizeof message, 0);
        event_set(&g_events[i], cfd, EPOLLIN, main_menu);
        event_add(&g_events[i]);
    } while (0);

    printf("new connect [%s:%d],[fd=%d] pos[%d]\n",
           inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), cfd, i);
}

void main_menu(int cfd, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    char *buf = ev->buf;
    int len = recv(cfd, buf, BUFLEN, 0);
    if (len <= 0)
    {
        close_cfd(cfd, (void *)ev, &len);
        return;
    }
    if (buf[0] == '1') // Anonymous
    {
        // TODO
    }
    else if (buf[0] == '2') // Login
    {
        ev->status = INPUT_ACC;
        strcpy(buf, "Enter account:");
        send(cfd, buf, strlen(buf), 0);

        event_set(ev, cfd, EPOLLIN, login);
        event_mod(ev);
    }
    else if (buf[0] == '3') // Sign up
    {
        ev->status = INPUT_ACC;
        ev->user = (struct user *)malloc(sizeof(struct user));
        memset(ev->user, 0, sizeof(struct user));
        strcpy(buf, "Register account (No more than 8 lowercase letters):");
        send(cfd, buf, strlen(buf), 0);

        event_set(ev, cfd, EPOLLIN, signUp);
        event_mod(ev);
    }
    else
    {
        // TODO
    }
}

void login(int cfd, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    char *buf = ev->buf;
    int len = recv(cfd, buf, BUFSIZ, 0);
    if (len <= 0)
    {
        close_cfd(cfd, (void *)ev, &len);
        return;
    }
    buf[len - 1] = '\0';

    char s[128];
    if (INPUT_ACC == ev->status) // get user account
    {
        struct TreeNode *res = findFromTireTree(user_tree, buf, NULL);
        if (!res)
        {
            sprintf(s, "User [%s] does not exist\nRe-enter account:", buf);
            send(cfd, s, strlen(s), 0);
            return;
        }
        ev->user = (struct user *)res->data;
        if (ev->user->online)
        {
            sprintf(s, "User [%s] has logged in\nRe-enter account:", buf);
            send(cfd, s, strlen(s), 0);
            return;
        }
        strcpy(buf, "Enter password:");
        send(cfd, buf, strlen(buf), 0);
    }
    else if (INPUT_PWD == ev->status) // get user password
    {
        if (!strcmp(buf, ev->user->pwd))
        {
            ev->user->online = true;
            ev->user->ev = ev;
            online_num++;
            sprintf(buf, "User [%s] log in, current online:[%d]\n\n>$ ", ev->user->acc, online_num);
            ev->len = strlen(buf);
            sprintf(s, "---------------- Online ChatRoom BETA ----------------[%s][%s]\n", ev->user->id, ev->user->acc);
            send(cfd, s, strlen(s), 0);

            event_set(ev, cfd, EPOLLOUT, send_data);
            event_mod(ev);
        }
        else
        {
            strcpy(buf, "Password incorrect\nRe-enter password:");
            send(cfd, buf, strlen(buf), 0);
            return;
        }
    }
    ev->status++;
    if (CHECK_PWD == ev->status)
        ev->status = ONLINE;
}

void signUp(int cfd, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    char *buf = ev->buf;
    int len = recv(cfd, buf, BUFSIZ, 0);
    if (len <= 0)
    {
        free(ev->user);
        ev->user = NULL;
        close_cfd(cfd, ev, &len);
        return;
    }
    buf[len - 1] = '\0';

    if (INPUT_ACC == ev->status) // get user account
    {
        struct TreeNode *res = findFromTireTree(user_tree, buf, NULL);
        if (res)
        {
            char s[128];
            sprintf(s, "Account [%s] has existed\nRe-enter account:", buf);
            send(cfd, s, strlen(s), 0);
            return;
        }
        strcpy(ev->user->acc, buf);
        strcpy(buf, "Enter password (No more than 8 characters):");
        send(cfd, buf, strlen(buf), 0);
    }
    else if (INPUT_PWD == ev->status) // get user password
    {
        strcpy(ev->user->pwd, buf);
        strcpy(buf, "Re-enter password:");
        send(cfd, buf, strlen(buf), 0);
    }
    else if (CHECK_PWD == ev->status) // check user password
    {
        if (strcmp(ev->user->pwd, buf))
        {
            strcpy(buf, "Passwords not match. Re-enter password:");
            send(cfd, buf, strlen(buf), 0);
            return;
        }
        sprintf(ev->user->id, "%05d", ++uid);
        FILE *fp = fopen(user_path, "a+");
        if (!fp)
        {
            send(ev->fd, "Register error\n", 6, 0);
            fprintf(stderr, "Sign up open file error\n");
        }
        fprintf(fp, "%s %s %s\n", ev->user->id, ev->user->acc, ev->user->pwd);
        fflush(fp);
        fclose(fp);
        insertToTireTree(user_tree, ev->user->acc, (void *)ev->user, sizeof(struct user));

        sprintf(buf, "Registration success, uid[%s], account[%s]\nReturn main menu...\n", ev->user->id, ev->user->acc);
        send(cfd, buf, strlen(buf), 0);
        send(cfd, message, sizeof message, 0);
        ev->user = NULL;

        event_set(ev, cfd, EPOLLIN, main_menu);
        event_mod(ev);
    }
    ev->status++;
    if (RESERVED == ev->status)
        ev->status = ONLINE;
}

void logout(int cfd, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    if (ev->status == ONLINE)
    {
        online_num--;
        char *buf = ev->buf;
        sprintf(buf, "User [%s] log out, current online:[%d]\n\n>$ ", ev->user->acc, online_num);
        ev->len = strlen(buf);
        send_data(cfd, ev);
    }
    ev->user->online = OFFLINE;
    ev->user->ev = NULL;
}

void close_cfd(int cfd, void *arg, int *len)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    if (ev->user)
        logout(cfd, (void *)ev);
    event_del(ev);
    Close(ev->fd);
    memset(ev, 0, sizeof(struct myevent_s));

    if (len) // recv
    {
        if (*len == 0)
            printf("[fd=%d] pos[%ld], closed\n", cfd, ev - g_events);
        else
            sys_error("recv error");
    }
    else // send
    {
        printf("send[fd=%d] error %s\n", cfd, strerror(errno));
    }
}

void recv_data(int fd, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    char tmp[BUFLEN - 64];
    int len = recv(fd, tmp, sizeof tmp, 0);

    if (len > 0)
    {
        tmp[len] = '\0';
        printf("receive from [fd=%d] pos[%ld],[len=%d]:%s", fd, ev - g_events, len - 1, tmp);
        sprintf(ev->buf, "(%s):%s\n>$ ", ev->user->acc, tmp); // format message
        ev->len = strlen(ev->buf);

        event_set(ev, fd, EPOLLOUT, send_data);
        event_mod(ev);
    }
    else
    {
        close_cfd(fd, (void *)ev, &len);
        return;
    }
}

void send_msg(struct TreeNode *node, void *arg)
{
    struct user *user = (struct user *)node->data;
    struct myevent_s *ev = (struct myevent_s *)arg;
    if (ev != user->ev && user->online)
        send(user->ev->fd, ev->buf, ev->len, 0);
}

void send_data(int fd, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    if (ev->len > 0)
    {
        travelTireTree(user_tree, send_msg, (void *)ev);
        if (ev->status == ONLINE)
            send(fd, "\n>$ ", 4, 0);
        event_set(ev, fd, EPOLLIN, recv_data);
        event_mod(ev);
    }
    else
    {
        close_cfd(fd, (void *)ev, NULL);
        return;
    }
}

void init_app()
{
    g_efd = epoll_create(MAX_EVENTS + 1);
    if (g_efd <= 0)
        printf("create g_efd in %s err %s\n", __func__, strerror(errno));
    uid = 0;
    online_num = 0;
}

void init_lfd()
{
    int lfd = Socket(AF_INET, SOCK_STREAM, 0);

    // fcntl(lfd, F_SETFL, O_NONBLOCK);
    // int opt = 1;
    // if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof opt) == -1)
    //     sys_error("setsockopt error");

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof sin);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(SERV_PROT);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    Bind(lfd, (struct sockaddr *)&sin, sizeof sin);

    Listen(lfd, 128);

    event_set(&g_events[MAX_EVENTS], lfd, EPOLLIN, accept_conn);
    event_add(&g_events[MAX_EVENTS]);
}

void init_user()
{
    user_tree = initTireTree(ALPHABET);
    FILE *fp = fopen(user_path, "r"); // open user message file
    if (fp == NULL)
        sys_error("load error");

    while (!feof(fp))
    {
        struct user *user = (struct user *)malloc(sizeof(struct user));
        memset(user, 0, sizeof(struct user));
        fscanf(fp, "%s %s %s", user->id, user->acc, user->pwd);
        int tmp = atoi(user->id);
        if (uid < tmp)
            uid = tmp;
        insertToTireTree(user_tree, user->acc, (void *)user, sizeof(struct user));
    }
    fclose(fp);
}

int main(void)
{
    init_app();
    init_lfd();
    init_user();

    struct epoll_event events[MAX_EVENTS + 1];
    printf("---------------------- Start Server ----------------------[OK]\n");
    int i, nready;
    // int checkpos = 0, i;
    while (1)
    {
        // long now = time(NULL);
        // for (i = 0; i < 100; i++, checkpos++)
        // {
        //     if (checkpos == MAX_EVENTS)
        //         checkpos = 0;
        //     if (g_events[checkpos].status != ONLINE)
        //         continue;
        //     long duration = now - g_events[checkpos].last_active;
        //     if (duration >= 60)
        //     {
        //         printf("[fd=%d] timeout\n", g_events[checkpos].fd);
        //         event_del(&g_events[checkpos]);
        //         Close(g_events[checkpos].fd);
        //     }
        // }

        nready = epoll_wait(g_efd, events, MAX_EVENTS + 1, 1000);
        if (nready == -1)
            sys_error("epoll wait error");
        for (i = 0; i < nready; i++)
        {
            struct myevent_s *ev = (struct myevent_s *)events[i].data.ptr;
            ev->callback(ev->fd, (void *)ev);
        }
    }

    return 0;
}
