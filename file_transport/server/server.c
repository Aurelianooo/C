#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include <arpa/inet.h>

int main(void){
    int port=12347;
    // 创建套接字
    int socket_fd=socket(AF_INET, SOCK_STREAM, 0);
    
    // 绑定套接字
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(struct sockaddr_in));  // 初始化
    server_addr.sin_family=AF_INET;  // 协议
    server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");  // 服务器IP
    server_addr.sin_port=htons((unsigned short)port);  // 服务器端口，转换字节顺序
    bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    // 监听
    listen(socket_fd, 5);
    
    // 等待客户端连接
    int connect_fd;
    connect_fd=accept(socket_fd, NULL, NULL);
    printf("connect accepted!\n");
    
    // 发送文件名
    char buf[256]="";
    write(connect_fd, "a.txt", 6);
    
    // 发送文件内容
    int fd=open("a.txt", O_RDONLY);
    int ret;
    while((ret=read(fd, buf, 256))>0){
    	printf("server send: %s\n", buf);
    	write(connect_fd, buf, ret);
    	memset(buf, 0, 256);
    }
    close(fd);
    
    close(socket_fd);
    return 0;
}
