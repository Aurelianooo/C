#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
// posix API gcc XXX -lpthread
// 10个线程， 每一个为变量 cnt 增加 10,000 ,最终结果应为 100,000
#define THREAD_SIZE 10
//#define MUTEX
//#define SPINLOCK
//#define ASM

#ifdef MUTEX
pthread_mutex_t mutex;
#endif
#ifdef SPINLOCK
pthread_spinlock_t spinlock;
#endif

int inc(int* value, int add){
	int old;
	
	__asm__ volatile(
		"lock; xaddl %2, %1;"
		: "=a"(old)
		: "m"(*value), "a"(add)
		: "cc", "memory"
	);
	
	return old;
}

void* func(void* arg){
	// int cnt=*(int*)arg;
	int* pcnt=(int*)arg;
	int i=0;
	while(i<10000){
	
	#ifdef MUTEX
		pthread_mutex_lock(&mutex);
		(*pcnt)++;
		pthread_mutex_unlock(&mutex);
	#else
	#ifdef SPINLOCK
		pthread_spin_lock(&spinlock);
		(*pcnt)++;
		pthread_spin_unlock(&spinlock);
	#else
	#ifdef ASM
		inc(pcnt, 1);
	#else
		(*pcnt)++;
	#endif
	#endif
	#endif
		
		usleep(1000);
		i++;
	}
}


int main(void){
    
    pthread_t thid[THREAD_SIZE]={0};
    int i, cnt=0;
    
    #ifdef MUTEX
    pthread_mutex_init(&mutex, NULL);
    #endif
    
    #ifdef SPINLOCK
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_SHARED);
    #endif
    
    for(i=0;i<THREAD_SIZE;i++){
    	pthread_create(&thid[i], NULL, func, &cnt);
    }
    for(i=0;i<20 ;i++){
    	printf("count %d: %d\n", i, cnt);
    	sleep(1);
    }

    return 0;
}
