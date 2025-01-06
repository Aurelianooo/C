#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<pthread.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<sys/time.h>
#include<sys/resource.h>

void createClient(int id, int myPort, int peerPort){
	int reuse=1;
	
	int socketFd;
	struct sockaddr_in peer_Addr;
	peer_Addr.sin_family=PF_INET;  // 使用IPv4协议
	peer_Addr.sin_port=htons(peerPort);  // 设置接收方端口号
	peer_Addr.sin_addr.s_addr=inet_addr("192.168.233.128");  // 设置接收方IP
	
	struct sockaddr_in self_Addr;
	self_Addr.sin_family=PF_INET;  // 使用IPv4协议
	self_Addr.sin_port=htons(myPort);  // 设置发送方端口号
	self_Addr.sin_addr.s_addr=inet_addr("0.0.0.0");  // 设置发送方IP
	
	if((socketFd=socket(PF_INET, SOCK_DGRAM|SOCK_CLOEXEC,0))==-1){
		perror("child socket");
		exit(1);
	}
	
	int opt=fcntl(socketFd, F_GETFL, 0);
	fcntl(socketFd, F_SETFL, opt|O_NONBLOCK);
	
	// 设置ip和端口复用
	if(setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))){
		exit(1);
	}
	if(setsockopt(socketFd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse))){
		exit(1);
	}
	
	// 绑定自己的IP地址
	if(bind(socketFd, (struct sockaddr*)&self_Addr, sizeof(struct sockaddr))){
		perror("chid bind");
		exit(1);
	}
	
	// 绑定对端ip和端口
	if(connect(socketFd, (struct sockaddr*)&peer_Addr, sizeof(struct sockaddr))){
		perror("chid connect");
		exit(1);
	}
	
	char* buffer="hello";
	sendto(socketFd, buffer, strlen(buffer), 0 ,(struct sockaddr*)&peer_Addr, sizeof(struct sockaddr));
	
}

void serial(int clientNum){
	for(int i=1;i<=clientNum;i++){
		createClient(i, 2025+i, 1234);
		//usleep(10);
	}
}

int main(void){

	// 多线程并发测试UDP服务
    serial(1024);
    
    printf("Serial done\n");
    return 0;
}
