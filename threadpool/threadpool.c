#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#define DEFAULT_TIME 10
#define MIN_WAIT_TASK_NUM 10
#define DEFAULT_THREAD_VARY 10

typedef struct
{
    void *(*function)(void *);
    void *arg;
} threadpool_task_t;

typedef struct
{
    pthread_mutex_t lock;           // 线程池锁
    pthread_mutex_t thread_counter; // 忙线程个数锁：busy_thr_num
    pthread_cond_t queue_not_full;  // 任务队列满条件变量
    pthread_cond_t queue_not_empty; // 任务队列空条件变量

    pthread_t *threads;            // 线程池数组
    pthread_t admin_tid;           // 管理线程
    threadpool_task_t *task_queue; // 任务队列

    int min_thr_num;       // 线程池最小线程数
    int max_thr_num;       // 线程池最大线程数
    int live_thr_num;      // 当前存活线程数
    int busy_thr_num;      // 忙线程数
    int wait_exit_thr_num; // 要销毁的线程数

    int queue_front;    // 任务队列头指针
    int queue_rear;     // 任务队列尾指针
    int queue_size;     // 任务队列实际任务数
    int queue_max_size; // 任务队列可容纳任务数上限

    bool shutdown; // 标志位，是否启用线程池
} threadpool_t;

void *threadpool_thread(void *threadpool);
void *adjust_thread(void *threadpool);

bool is_thread_alive(pthread_t tid);
int threadpool_free(threadpool_t *pool);

threadpool_t *threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size)
{
    int i;
    threadpool_t *pool = NULL;

    do
    {
        if ((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL)
        {
            printf("malloc threadpool fail");
            break;
        }

        pool->min_thr_num = min_thr_num;
        pool->max_thr_num = max_thr_num;
        pool->busy_thr_num = 0;
        pool->live_thr_num = min_thr_num;
        pool->wait_exit_thr_num = 0;
        pool->queue_size = 0;
        pool->queue_max_size = queue_max_size;
        pool->queue_front = 0;
        pool->queue_rear = 0;
        pool->shutdown = false;

        // 为线程数组开辟空间并清零
        pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * max_thr_num);
        if (pool->threads == NULL)
        {
            printf("malloc threads fail");
            break;
        }
        memset(pool->threads, 0, sizeof(pthread_t) * max_thr_num);

        // 为任务队列开辟空间
        pool->task_queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_max_size);
        if (pool->task_queue == NULL)
        {
            printf("malloc task_queue fail");
            break;
        }

        // 初始化互斥锁、条件变量
        if (pthread_mutex_init(&(pool->lock), NULL) != 0 || pthread_mutex_init(&(pool->thread_counter), NULL) != 0 || pthread_cond_init(&(pool->queue_not_empty), NULL) != 0 || pthread_cond_init(&(pool->queue_not_full), NULL) != 0)
        {
            printf("init the lock or cond fail");
            break;
        }

        // 启动 min_thr_num 个工作线程
        for (i = 0; i < min_thr_num; i++)
        {
            pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
            printf("start thread 0x%x...\n", (unsigned int)pool->threads[i]);
        }
        pthread_create(&(pool->admin_tid), NULL, adjust_thread, (void *)pool);
        printf("start Admin thread 0x%x...\n", (unsigned int)pool->admin_tid);

        return pool;

    } while (0);

    threadpool_free(pool);
    return NULL;
}

// 向线程池中添加一个任务
int threadpool_add(threadpool_t *pool, void *(*function)(void *arg), void *arg)
{
    pthread_mutex_lock(&(pool->lock));
    // 任务队列满，阻塞
    while ((pool->queue_size == pool->queue_max_size) && (!pool->shutdown))
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));

    if (pool->shutdown)
    {
        pthread_cond_broadcast(&(pool->queue_not_empty));
        pthread_mutex_unlock(&(pool->lock));
        return 0;
    }

    // 清空工作线程调用的回调函数参数arg
    if (pool->task_queue[pool->queue_rear].arg != NULL)
        pool->task_queue[pool->queue_rear].arg = NULL;

    // 添加任务到任务队列中
    pool->task_queue[pool->queue_rear].function = function;
    pool->task_queue[pool->queue_rear].arg = arg;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_max_size;
    pool->queue_size++;

    // 添加后任务队列不为空，唤醒线程池中阻塞的线程
    pthread_cond_signal(&(pool->queue_not_empty));
    pthread_mutex_unlock(&(pool->lock));

    return 0;
}

// 线程池中的各个工作线程
void *threadpool_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;

    while (true)
    {
        // 刚创建线程，等待任务队列中添加任务，任务队列为空则阻塞
        pthread_mutex_lock(&(pool->lock));

        // queue_size == 0 说明没有任务，阻塞在条件变量上
        while ((pool->queue_size == 0) && (!pool->shutdown))
        {
            printf("thread 0x%x is waiting\n", (unsigned int)pthread_self());
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));

            // 清理指定数量的空闲线程，如果要结束的线程个数大于0,结束线程
            if (pool->wait_exit_thr_num > 0)
            {
                pool->wait_exit_thr_num--;
                // 如果线程池中线程个数大于最小值，可以结束当前线程
                if (pool->live_thr_num > pool->min_thr_num)
                {
                    printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
                    pool->live_thr_num--;
                    pthread_mutex_unlock(&(pool->lock));
                    pthread_exit(NULL);
                }
            }
        }

        // 如果要销毁线程池，需要关闭其中每个线程，自行退出处理
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&(pool->lock));
            printf("Thread Pool shutting down.\n");
            printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
            pthread_detach(pthread_self());
            pthread_exit(NULL); // 线程自行结束
        }

        // 从任务队列取出一个任务
        task.function = pool->task_queue[pool->queue_front].function;
        task.arg = pool->task_queue[pool->queue_front].arg;

        pool->queue_front = (pool->queue_front + 1) % pool->queue_max_size;
        pool->queue_size--;

        // 通知其他任务可以添加到任务队列中
        pthread_cond_signal(&(pool->queue_not_full));
        // 取出任务后释放线程池锁
        pthread_mutex_unlock(&(pool->lock));

        // 处理任务
        printf("thread 0x%x is working\n", (unsigned int)pthread_self());
        pthread_mutex_lock(&(pool->thread_counter));
        pool->busy_thr_num++;
        pthread_mutex_unlock(&(pool->thread_counter));
        (*(task.function))(task.arg);

        // 结束任务
        printf("thread 0x%x finish\n", (unsigned int)pthread_self());
        pthread_mutex_lock(&(pool->thread_counter));
        pool->busy_thr_num--;
        pthread_mutex_unlock(&(pool->thread_counter));
    }
    pthread_exit(NULL);
}

void *adjust_thread(void *threadpool)
{
    int i;
    threadpool_t *pool = (threadpool_t *)threadpool;

    while (!pool->shutdown)
    {
        sleep(DEFAULT_TIME);

        pthread_mutex_lock(&(pool->lock));
        int queue_size = pool->queue_size;
        int live_thr_num = pool->live_thr_num;
        pthread_mutex_unlock(&(pool->lock));

        pthread_mutex_lock(&(pool->thread_counter));
        int busy_thr_num = pool->busy_thr_num;
        pthread_mutex_unlock(&(pool->thread_counter));

        // 创建新线程算法：任务数量大于最小线程数且存活线程数少于最大线程数
        if (queue_size >= MIN_WAIT_TASK_NUM && live_thr_num < pool->max_thr_num)
        {
            pthread_mutex_lock(&(pool->lock));
            int cnt = 0;
            // 每次增加 DEFAULT_THREAD_VARY 个线程
            for (i = 0; i < pool->max_thr_num && cnt < DEFAULT_THREAD_VARY && pool->live_thr_num < pool->max_thr_num; i++)
            {
                if (pool->threads[i] == 0 || !pthread_kill(pool->threads[i], 0))
                {
                    pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
                    cnt++;
                    pool->live_thr_num++;
                }
            }
            pthread_mutex_unlock(&(pool->lock));
        }

        // 销毁多余空闲线程算法：忙线程x2小于存活线程数且存活线程数大于最小线程数
        if ((busy_thr_num * 2) < live_thr_num && live_thr_num > pool->min_thr_num)
        {
            // 每次随机销毁 DEFAULT_THREAD_VARY 个线程
            pthread_mutex_lock(&(pool->lock));
            pool->wait_exit_thr_num = DEFAULT_THREAD_VARY;
            pthread_mutex_unlock(&(pool->lock));

            for (i = 0; i < DEFAULT_THREAD_VARY; i++)
                // 通知处于空闲状态的线程，它们会自行终止
                pthread_cond_signal(&(pool->queue_not_empty));
        }
    }
    return NULL;
}

int threadpool_destroy(threadpool_t *pool)
{
    int i;
    if (!pool)
        return -1;
    pool->shutdown = true;

    // 先销毁管理线程
    pthread_join(pool->admin_tid, NULL);
    for (i = 0; i < pool->live_thr_num; i++)
        // 通知所有空闲线程
        pthread_cond_broadcast(&(pool->queue_not_empty));

    for (i = 0; i < pool->live_thr_num; i++)
        pthread_join(pool->threads[i], NULL);

    threadpool_free(pool);
    return 0;
}

int threadpool_free(threadpool_t *pool)
{
    if (!pool)
        return -1;
    if (pool->task_queue)
        free(pool->task_queue);
    if (pool->threads)
    {
        free(pool->threads);
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_mutex_lock(&(pool->thread_counter));
        pthread_mutex_destroy(&(pool->thread_counter));
        pthread_cond_destroy(&(pool->queue_not_empty));
        pthread_cond_destroy(&(pool->queue_not_full));
    }
    free(pool);
    pool = NULL;
    printf("Thread Pool closed.\n");
    return 0;
}

// 线程池中的线程，模拟处理业务
void *process(void *arg)
{
    printf("thread 0x%x working on task %d\n", (unsigned int)pthread_self(), (*(int *)arg));
    sleep(1);
    printf("task %d is done\n", (*(int *)arg));
    return NULL;
}

int main(void)
{
    threadpool_t *thp = threadpool_create(4, 64, 64);
    printf("Thread Pool initialized......\n");

    int num[20], i;
    for (i = 0; i < 20; i++)
    {
        num[i] = i;
        printf("add task %d\n", i);

        threadpool_add(thp, process, (void *)&num[i]);
    }

    sleep(DEFAULT_TIME);
    threadpool_destroy(thp);

    return 0;
}
