#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SIZE 1024

extern int errno;

int main(void){
    int shmid;
    char *shmptr;
    key_t key;
    pid_t pid;

    if((pid = fork()) < 0){
        fprintf(stdout, "fork error:%s\n", strerror(errno));
        return -1;
    } else if(pid == 0){ // child
        // wait for father create share memory
        sleep(1);
        
        if((key = ftok("./test", 1)) < 0){
            fprintf(stdout, "child ftok error:%s\n", strerror(errno));
            return -1;
        }
        // share memory created by father, just get it.
        if((shmid = shmget(key, SIZE, 0600)) < 0){
            fprintf(stdout, "child shmget error:%s\n", strerror(errno));
            exit(-1);
        }
        if((shmptr = (char *)shmat(shmid, 0, 0)) == (void *) -1){
            fprintf(stdout, "child shmat error:%s\n", strerror(errno));
            exit(-1);
        }

        printf("child pid is %d, share memory from %lx to %lx, content:%s\n", 
                getpid(), (unsigned long)shmptr, (unsigned long)(shmptr + SIZE), shmptr);
        printf("child process sleep 2 seconds\n");
        sleep(2);

        exit(0);
    } else {    // father
        const char *str = "Hello world!";

        if((key = ftok("./test", 1)) < 0){
            fprintf(stdout, "parent ftok error:%s\n", strerror(errno));
            return -1;
        }
        if((shmid = shmget(key, SIZE, 0600 | IPC_CREAT | IPC_EXCL)) < 0){
            fprintf(stdout, "parent shmget error:%s\n", strerror(errno));
            exit(-1);
        }
        if((shmptr = (char *)shmat(shmid, 0, 0)) == (void *) -1){
            fprintf(stdout, "parent shmat error:%s\n", strerror(errno));
            exit(-1);
        }

        memcpy(shmptr, str, strlen(str) + 1);
        printf("parent pid is %d, share memory from %lx to %lx, content:%s\n", 
                getpid(), (unsigned long)shmptr, (unsigned long)(shmptr + SIZE), shmptr);
        printf("parent process sleep 2 seconds\n");
        sleep(2);

        // father remove share memory, child no need to do it again.
        if(shmctl(shmid, IPC_RMID, 0) < 0){
            fprintf(stdout, "parent shmctl error:%s\n", strerror(errno));
            exit(-1);
        }
        waitpid(pid, NULL, 0);
        exit(0);
    }

    return 0;
}
