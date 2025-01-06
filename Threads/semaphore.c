// 信号量实现生产者消费者问题
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<semaphore.h>

#define NUM 5

int queue[NUM];
sem_t blank_number, product_number;

void* producer(void* arg){
    int i=0;
    while (1){
        sem_wait(&blank_number);
        queue[i]= rand()%1000+1;
        printf("------Producer---%d------\n", queue[i]);
        sem_post(&product_number);

        i=(i+1)%NUM;
        sleep(rand()%1);
    }
}

void * consumer(void* arg){
    int i=0;
    while (1){
        sem_wait(&product_number);
        printf("Consumer---%d\n", queue[i]);
        queue[i]=0;
        sem_post(&blank_number);

        i=(i+1)%NUM;
        sleep(rand()%3);
    }
}

int main(void){
    pthread_t pid, cid;
    sem_init(&blank_number, 0, NUM);  // 初始化空格子信号量为5， 线程间共享
    sem_init(&product_number, 0, 0);  // 初始化产品信号量为0

    pthread_create(&pid, NULL, producer, NULL);
    pthread_create(&cid, NULL, consumer, NULL);

    pthread_join(pid, NULL);
    pthread_join(cid, NULL);

    return 0;
}
