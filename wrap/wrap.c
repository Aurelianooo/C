
#include "wrap.h"

void sys_error(const char *str)
{
	perror(str);
	exit(-1);
}

int Accept(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	int n;
again:
	if ((n = accept(fd, addr, addrlen)) < 0)
	{
		if ((errno == ECONNABORTED) || (errno == EINTR))
			goto again;
		else
			sys_error("accept error");
	}
	return n;
}

int Bind(int fd, const struct sockaddr *addr, socklen_t addrlen)
{
	int n;
	if ((n = bind(fd, addr, addrlen)) < 0)
		sys_error("bind error");
	return n;
}

int Connect(int fd, const struct sockaddr *addr, socklen_t addrlen)
{
	int n;
	if ((n = connect(fd, addr, addrlen)) < 0)
		sys_error("connect error");
	return n;
}

int Listen(int fd, int backlog)
{
	int n;
	if ((n = listen(fd, backlog)) < 0)
		sys_error("listen error");
	return n;
}

int Socket(int family, int type, int protocol)
{
	int n;
	if ((n = socket(family, type, protocol)) < 0)
		sys_error("socket error");
	return n;
}

ssize_t Read(int fd, void *ptr, size_t nbytes)
{
	ssize_t n;
again:
	if ((n = read(fd, ptr, nbytes)) == -1)
	{
		if (errno == EINTR)
			goto again;
		else
			return -1;
	}
	return n;
}

ssize_t Write(int fd, const void *ptr, size_t nbytes)
{
	ssize_t n;
again:
	if ((n = write(fd, ptr, nbytes)) == -1)
	{
		if (errno == EINTR)
			goto again;
		else
			return -1;
	}
	return n;
}

int Close(int fd)
{
	int n;
	if ((n = close(fd)) == -1)
		sys_error("close error");
	return n;
}

// third param: bytes to read
ssize_t Readn(int fd, void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nread;
	char *ptr;
	ptr = vptr;
	nleft = n;

	while (nleft > 0)
	{
		if ((nread = read(fd, ptr, nleft)) < 0)
		{
			if (errno == EINTR)
				nread = 0;
			else
				return -1;
		}
		else if (nread == 0)
			break;
		nleft -= nread;
		ptr += nread;
	}
	return n - nleft;
}

ssize_t Writen(int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;
	ptr = vptr;
	nleft = n;

	while (nleft > 0)
	{
		if ((nwritten = write(fd, ptr, nleft)) <= 0)
		{
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;
			else
				return -1;
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return n;
}

ssize_t my_read(int fd, char *ptr)
{
	static int read_cnt;
	static char *read_ptr;
	static char read_buf[100];

	if (read_cnt <= 0)
	{
	again:
		if ((read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0)
		{
			if (errno == EINTR)
				goto again;
			return -1;
		}
		else if (read_cnt == 0)
			return 0;
		read_ptr = read_buf;
	}
	read_cnt--;
	*ptr = *read_ptr++;

	return 1;
}

// readline --- fgets
ssize_t Readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t n, rc;
	char c, *ptr;
	ptr = vptr;

	for (n = 0; n < maxlen; n++)
	{
		if ((rc = my_read(fd, &c)) == 1)
		{
			*ptr++ = c;
			if (c == '\n')
				break;
		}
		else if (rc == 0)
		{
			*ptr = 0;
			return n - 1;
		}
		else
			return -1;
	}
	*ptr = 0;

	return n;
}
