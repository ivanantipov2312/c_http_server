#ifndef TPOOL_H_
#define TPOOL_H_
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

typedef void *(* thread_func_t)(void *);

struct tpool {
	struct tpool_job *head;
	struct tpool_job *tail;

	pthread_mutex_t job_mutex;
	pthread_cond_t job_cond;
	pthread_cond_t working_cond;

	size_t num_threads;
	size_t num_working;
	bool stop;
};


struct tpool *tpool_create(size_t num_threads);
void tpool_destroy(struct tpool *pool);

bool tpool_add_work(struct tpool *pool, thread_func_t func, void *arg);

#endif // TPOOL_H_
