
#ifndef __WRAP_H_
#define __WRAP_H_
#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

void sys_error(const char *str);

int Accept(int fd, struct sockaddr *addr, socklen_t *addrlen);

int Bind(int fd, const struct sockaddr *addr, socklen_t addrlen);

int Connect(int fd, const struct sockaddr *addr, socklen_t addrlen);

int Listen(int fd, int backlog);

int Socket(int family, int type, int protocol);

ssize_t Read(int fd, void *ptr, size_t nbytes);

ssize_t Write(int fd, const void *ptr, size_t nbytes);

int Close(int fd);

ssize_t Readn(int fd, void *vptr, size_t n);

ssize_t Writen(int fd, const void *vptr, size_t n);

ssize_t my_read(int fd, char *ptr);

ssize_t Readline(int fd, void *vptr, size_t maxlen);

#endif
