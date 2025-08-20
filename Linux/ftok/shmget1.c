#include <stdio.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define SIZE 1024

extern int errno;

int main(void){
    int shmid;
    key_t key;
    char *shmptr = NULL;
    const char *str = "Hello world!";

    if((key = ftok("./test", 1)) < 0){
        fprintf(stdout, "child ftok error:%s\n", strerror(errno));
        return -1;
    }

    // 创建共享内存
    if((shmid = shmget(key, SIZE, 0600 | IPC_CREAT))<0){
        printf("shmget error:%s\n", strerror(errno));
        return -1;
    }
    printf("get share memory ID:%d\n", shmid);

    // 将共享内存连接到可用地址上
    if((shmptr = (char *)shmat(shmid, 0, 0)) == (void *)-1){
        printf("shmat error:%s\n", strerror(errno));
        return -1;
    }

    memcpy(shmptr, str, strlen(str)+1);
    printf("share memory from %lx to %lx, content:%s\n", (unsigned long)shmptr, (unsigned long)(shmptr + SIZE), shmptr);

    // 拆卸共享内存
    if(shmctl(shmid, IPC_RMID, 0)<0){
        printf("shmctl error:%s\n", strerror(errno));
        return -1;
    }

    return 0;
}
