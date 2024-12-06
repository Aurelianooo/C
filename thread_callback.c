#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

// 定义线程回调函数
void* call_back(void* arg) {
    int* num = (int*)arg; // 将 void* 转换为具体类型
    printf("Thread is running with argument: %d\n", *num);

    // 返回一个结果
    int* result = (int*)malloc(sizeof(int));
    *result = (*num) * 2;
    return (void*)result;
}

int main() {
    pthread_t thread;
    int arg = 10;

    // 创建线程
    if (pthread_create(&thread, NULL, call_back, (void*)&arg) != 0) {
        perror("Failed to create thread");
        return 1;
    }

    // 等待线程完成
    void* ret_val;
    if (pthread_join(thread, &ret_val) != 0) {
        perror("Failed to join thread");
        return 1;
    }

    // 处理线程返回值
    int* result = (int*)ret_val;
    printf("Thread returned: %d\n", *result);
    free(result); // 释放内存

    return 0;
}

