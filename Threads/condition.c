#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>

#define BUFFER_SIZE 4

//创建一把互斥锁和一个条件变量
pthread_mutex_t mutex;
pthread_cond_t cond;

//环形缓冲区，用于放置生产好的产品
typedef struct{
	int len;
	int head;
	int tail;
	int maxSize;
	int* queue;
} Ring_Buffer;

bool Empty(Ring_Buffer* q){
	return q->len==0 ? true:false;
}

bool Full(Ring_Buffer* q){
	return q->len==q->maxSize ? true:false;
}

//扩展缓冲区，每次增加BUFFER_SIZE大小
int Expand(Ring_Buffer* q){
	q->maxSize=q->maxSize+BUFFER_SIZE;
	q->queue=realloc(q->queue, q->maxSize*sizeof(int));
	if(q->queue==NULL)	return -1;
	return q->maxSize;
}

Ring_Buffer* creat_buffer(){
	printf("创建环形缓冲区.....\n");
	Ring_Buffer* q=(Ring_Buffer*)malloc(sizeof(Ring_Buffer));
	if(q==NULL){
		printf("缓冲区创建失败\n");
		exit(-1);
	}
	q->queue=(int*)malloc(BUFFER_SIZE*sizeof(int));
	if(q->queue==NULL){
		printf("缓冲区创建失败\n");
		exit(-1);
	}
	memset(q->queue, 0, sizeof(q->queue));
	q->head=0;
	q->tail=0;
	q->len=0;
	q->maxSize=BUFFER_SIZE;
	return q;
}

//生产者线程函数
void* creat_fun(void* arg)
{
    while(1){
        printf("生产者正在生产.....\n");
        //假设生产者1s生成一个
        sleep(1);
        //上锁
        pthread_mutex_lock(&mutex);
        Ring_Buffer* q=(Ring_Buffer*)arg;
        printf("生产者往仓库里面放了一个产品，放之前数目为%d\n", q->len);
    	q->len++;
    	if(Full(q)){
        	printf("缓冲区已满，扩展缓冲区\n");
        	int ret=Expand(q);
        	if(ret==-1){
				printf("缓冲区扩展失败\n");
				exit(-1);
			}
        }
    	q->queue[q->tail]=1;
    	q->tail=(q->tail+1)%q->maxSize;
        printf("当前缓冲区状态：");
        for(int i=0;i<q->maxSize;i++) printf("%d ",q->queue[i]);
        printf("\n");
        //唤醒休眠线程
        pthread_cond_broadcast(&cond);  // 可能会出现惊群现象
        //pthread_cond_signal(&cond);  // 只会唤醒一个消费者
        //解锁
        pthread_mutex_unlock(&mutex);
    }
 
    return NULL;
}
 
//消费者1线程创建
void* custmer1_fun(void* arg)
{
    while(1){
        //上锁
        pthread_mutex_lock(&mutex);
        Ring_Buffer* q=(Ring_Buffer*)arg;
        //条件变量创建，如果没有产品只能等待条件满足
        if(Empty(q)){
            printf("消费者1发现没有产品,等待生产\n");
            pthread_cond_wait(&cond,&mutex);
        }else{
            printf("消费者1进入仓库买产品,当前剩余%d个产品\n",q->len);
            q->queue[q->head]=0;
            q->head=(q->head+1)%q->maxSize;
            q->len--;
            printf("当前缓冲区状态：");
            for(int i=0;i<q->maxSize;i++) printf("%d ",q->queue[i]);
            printf("\n");
            printf("消费者1正在使用产品\n");           
        }    
        //解锁
        pthread_mutex_unlock(&mutex);
        //消费者1使用产品需要2s
        sleep(2);
    }
    return NULL; 
}

//消费者2线程创建
void* custmer2_fun(void* arg)
{
    while(1){
        //上锁
        pthread_mutex_lock(&mutex);
        Ring_Buffer* q=(Ring_Buffer*)arg;
        //条件变量创建，如果没有产品只能等待条件满足
        if(Empty(q)){
            printf("消费者2发现没有产品,等待生产\n");
            pthread_cond_wait(&cond,&mutex);
        }
        else{
            printf("消费者2进入仓库买产品,当前剩余%d个产品\n",q->len);
            q->queue[q->head]=0;
            q->head=(q->head+1)%q->maxSize;
            q->len--;
            printf("当前缓冲区状态：");
            for(int i=0;i<q->maxSize;i++) printf("%d ",q->queue[i]);
            printf("\n");
            printf("消费者2正在使用产品\n");         
        } 
        //解锁
        pthread_mutex_unlock(&mutex);
        //消费者2使用产品需要3s
        sleep(3);
    }
    return NULL; 
}
 
 
int main(int argc, char const *argv[])
{
    //初始化锁,条件变量
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond,NULL);
    
    //创建环形缓冲区
    Ring_Buffer* rb=creat_buffer();
    
    //创建三个线程，两个消费者一个生产者
    pthread_t cus1 , cus2 , creat;
    pthread_create(&creat, NULL, creat_fun , rb);
    pthread_create(&cus1, NULL, custmer1_fun , rb);
    pthread_create(&cus2, NULL, custmer2_fun , rb);
    //回收线程
    pthread_join(creat, NULL);
    pthread_join(cus1, NULL);
    pthread_join(cus2, NULL);
    
    //销毁锁,条件变量
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return 0;
}

