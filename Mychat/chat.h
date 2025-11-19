#ifndef _MY_CHAT_H_
#define _MY_CHAT_H_

#include "../wrap/wrap.h"
#include "../Tire/tire.h"
#include <time.h>

#define MAX_EVENTS 1024
#define BUFLEN 1024
#define SERV_PROT 8888
#define ALPHABET 26 // lowercase letters only
#define UID_LEN 6   // length of user ID
#define USER_LEN 9  // length of user account and password

enum status
{
    OFFLINE,
    ONLINE,
    INPUT_ACC,
    INPUT_PWD,
    CHECK_PWD,
    RESERVED
};

struct user;

struct myevent_s
{
    int fd;
    int events;
    void (*callback)(int fd, void *arg);
    enum status status;
    char buf[BUFLEN];
    int len;
    long last_active;
    struct user *user;
};

struct user
{
    char id[UID_LEN];   // UID
    char acc[USER_LEN]; // account
    char pwd[USER_LEN]; // password
    bool online;
    struct myevent_s *ev;
};

void event_set(struct myevent_s *ev, int fd, int events, void (*callback)(int, void *));
void event_add(struct myevent_s *ev);
void event_del(struct myevent_s *ev);
void event_mod(struct myevent_s *ev);
void accept_conn(int lfd, void *arg);
void main_menu(int cfd, void *arg);
void login(int cfd, void *arg);
void signUp(int cfd, void *arg);
void logout(int cfd, void *arg);
void close_cfd(int cfd, void *arg, int *len);
void recv_data(int fd, void *arg);
void send_msg(struct TreeNode *node, void *arg);
void send_data(int fd, void *arg);
void init_app();
void init_lfd();
void init_user();

#endif
