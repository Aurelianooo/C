// 信号量实现生产者消费者问题
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define NUM 5

int queue[NUM];
int j = 0;  // 消费者共享
pthread_mutex_t mutex;
sem_t blank_number, product_number;

void *producer(void *arg)
{
    int i = 0;
    while (1)
    {
        sem_wait(&blank_number);
        pthread_mutex_lock(&mutex);
        queue[i] = rand() % 1000 + 1;
        printf("------Producer---%d------\n", queue[i]);
        pthread_mutex_unlock(&mutex);
        sem_post(&product_number);

        i = (i + 1) % NUM;
        sleep(rand() % 1);
    }
}

void *consumer1(void *arg)
{
    while (1)
    {
        sem_wait(&product_number);
        pthread_mutex_lock(&mutex);
        printf("Consumer---1---%d\n", queue[j]);
        queue[j] = 0;
        pthread_mutex_unlock(&mutex);
        sem_post(&blank_number);

        j = (j + 1) % NUM;
        sleep(rand() % 3);
    }
}

void *consumer2(void *arg)
{
    while (1)
    {
        sem_wait(&product_number);
        pthread_mutex_lock(&mutex);
        printf("Consumer---2---%d\n", queue[j]);
        queue[j] = 0;
        pthread_mutex_unlock(&mutex);
        sem_post(&blank_number);

        j = (j + 1) % NUM;
        sleep(rand() % 3);
    }
}

int main(void)
{
    pthread_t pid, cid1,cid2;

    pthread_mutex_init(&mutex, NULL);
    sem_init(&blank_number, 0, NUM); // 初始化空格子信号量为5， 线程间共享
    sem_init(&product_number, 0, 0); // 初始化产品信号量为0

    pthread_create(&pid, NULL, producer, NULL);
    pthread_create(&cid1, NULL, consumer1, NULL);
    pthread_create(&cid2, NULL, consumer2, NULL);

    pthread_join(pid, NULL);
    pthread_join(cid1, NULL);
    pthread_join(cid2, NULL);

    return 0;
}
