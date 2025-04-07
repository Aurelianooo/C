/*
 * WHAT THIS EXAMPLE DOES
 *
 * We create a pool of 4 threads and then add 20 tasks to the pool.
 * Task simply print which thread is running them.
 *
 * As soon as we add the tasks to the pool, the threads will run them. It can happen that
 * you see a single thread running all the tasks (highly unlikely). It is up the OS to
 * decide which thread will run what. So it is not an error of the thread pool but rather
 * a decision of the OS.
 *
 * */

#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include "thpool.h"

void task(void *arg)
{
	// printf("Thread #%u working on Task %ld\n", (int)pthread_self(), (uintptr_t)arg);
	printf("working on Task %ld\n", (uintptr_t)arg);
	sleep(1);
	printf("Task %ld done\n", (uintptr_t)arg);
}

int main()
{

	puts("Making threadpool with 4 threads");
	threadpool thpool = thpool_init(4);

	puts("Adding 20 tasks to jobqueue");
	int i;
	for (i = 0; i < 20; i++)
	{
		thpool_add_work(thpool, task, (void *)(uintptr_t)i);
	};

	thpool_wait(thpool);
	puts("Killing threadpool");
	thpool_destroy(thpool);

	return 0;
}
