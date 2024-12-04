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
    
    // 连接到服务器
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(struct sockaddr_in));  // 初始化
    server_addr.sin_family=AF_INET;  // 协议
    server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");  // 服务器IP
    server_addr.sin_port=htons((unsigned short)port);
    connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    char buf[256]="";
    read(socket_fd, buf, 256);
    printf("filename: %s\n", buf);
    int fd=creat(buf, O_WRONLY);
    memset(buf, 0, 256);
    int ret;
    // 这里要有括号，保证先赋值再判断大小
    while((ret=read(socket_fd, buf, 256))>0){
    	printf("client recv: %s\n", buf);
    	write(fd, buf, ret);
    	memset(buf, 0, 256);
    }
    close(fd);
    
    close(socket_fd);
    return 0;
}
