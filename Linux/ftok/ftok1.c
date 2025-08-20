#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main(void){
    // 生成唯一的 IPC 键
    key_t key = ftok("./test", 1);
    if(key == -1){
        perror("ftok error");
        return 1;
    }

    // 使用生成的键创建共享内存段
    int shmid = shmget(key, 1024, 0600 | IPC_CREAT);
    if(shmid == -1){
        perror("shmget error");
        return 1;
    }

    printf("Shared memory ID: % d\n", shmid);

    if(shmctl(shmid, IPC_RMID, 0)<0){
        perror("shmctl error");
        return 1;
    }
    return 0;
}
