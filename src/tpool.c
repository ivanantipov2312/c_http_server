#include <stdlib.h>
#include "tpool.h"

struct tpool_job {
	thread_func_t func;
	void *arg;
	struct tpool_job *next;
};

static struct tpool_job *tpool_job_create(thread_func_t func, void *arg)
{
	struct tpool_job *job = malloc(sizeof *job);
	job->func = func;
	job->arg = arg;
	job->next = NULL;
	return job;
}

static struct tpool_job *tpool_job_get(struct tpool *pool)
{
	if (pool == NULL)
		return NULL;

	struct tpool_job *job = pool->head;
	if (job == NULL)
		return NULL;

	if (job->next == NULL) {
		pool->head = NULL;
		pool->tail = NULL;
	} else {
		pool->head = job->next;
	}

	return job;
}

static void *worker(void *arg)
{
	struct tpool *pool = arg;
	while (1) {
		pthread_mutex_lock(&pool->job_mutex);

		while (pool->head == NULL && !pool->stop)
			pthread_cond_wait(&pool->job_cond, &pool->job_mutex);

		if (pool->stop)
			break;

		struct tpool_job *job = tpool_job_get(pool);
		pool->num_working++;
		pthread_mutex_unlock(&pool->job_mutex);

		if (job != NULL) {
			job->func(job->arg);
			free(job);
		}

		pthread_mutex_lock(&pool->job_mutex);
		pool->num_working--;
		if (!pool->stop && pool->num_working == 0 && pool->head == NULL)
			pthread_cond_signal(&pool->working_cond);
		pthread_mutex_unlock(&pool->job_mutex);

		pthread_mutex_unlock(&pool->job_mutex);
	}

	pool->num_threads--;
	pthread_cond_signal(&pool->working_cond);
	pthread_mutex_unlock(&pool->job_mutex);
	return NULL;
}

static void tpool_wait(struct tpool *pool)
{
	if (pool == NULL)
		return;

	pthread_mutex_lock(&pool->job_mutex);
	while (1) {
		if (pool->head != NULL || (!pool->stop && pool->num_working != 0) || (pool->stop && pool->num_threads != 0))
			pthread_cond_wait(&pool->working_cond, &pool->job_mutex);
		else
			break;
	}
	pthread_mutex_unlock(&pool->job_mutex);
}

struct tpool *tpool_create(size_t num_threads)
{
	if (num_threads == 0)
		num_threads = 2;

	struct tpool *pool = malloc(sizeof *pool);
	pool->num_threads = num_threads;

	pthread_mutex_init(&pool->job_mutex, NULL);
	pthread_cond_init(&pool->job_cond, NULL);
	pthread_cond_init(&pool->working_cond, NULL);

	pool->head = NULL;
	pool->tail = NULL;

	pthread_t tid;
	for (int i = 0; i < num_threads; i++) {
		pthread_create(&tid, NULL, worker, pool);
		pthread_detach(tid);
	}

	return pool;
}

void tpool_destroy(struct tpool *pool)
{
	if (pool == NULL)
		return;

	pthread_mutex_lock(&pool->job_mutex);
	struct tpool_job *job = pool->head;
	struct tpool_job *job_next;
	while (job != NULL) {
		job_next = job->next;
		free(job);
		job = job_next;
	}

	pool->head = NULL;
	pool->stop = true;

	pthread_cond_broadcast(&pool->job_cond);
	pthread_mutex_unlock(&pool->job_mutex);

	tpool_wait(pool);

	pthread_mutex_destroy(&pool->job_mutex);
	pthread_cond_destroy(&pool->job_cond);
	pthread_cond_destroy(&pool->working_cond);

	free(pool);
}

bool tpool_add_work(struct tpool *pool, thread_func_t func, void *arg)
{
	if (pool == NULL)
		return false;

	struct tpool_job *job = tpool_job_create(func, arg);

	pthread_mutex_lock(&pool->job_mutex);

	if (pool->head == NULL) {
		pool->head = job;
		pool->tail = job;
	} else {
		pool->tail->next = job;
		pool->tail = job;
	}

	pthread_cond_broadcast(&pool->job_cond);
	pthread_mutex_unlock(&pool->job_mutex);

	return true;
}
