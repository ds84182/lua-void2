#include "void_queue.h"

#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// Global Queue List
static pthread_mutex_t gql_lock = PTHREAD_MUTEX_INITIALIZER;
static void_queue **gql = 0;
static unsigned int gql_count = 0;

int void_queue_init(void_queue *queue, unsigned int size, const char *name) {
	memset(queue, 0, sizeof(void_queue));

	queue->refcount = 1;

	if (pthread_mutex_init(&queue->lock, 0)) {
		fprintf(stderr, "Could not initialize mutex\n");
		return 0;
	}

	if (pthread_cond_init(&queue->cond, 0)) {
		fprintf(stderr, "Could not initialize condition variable\n");
		pthread_mutex_destroy(&queue->lock);
		return 0;
	}

	queue->size = size;
	queue->buffers = malloc(sizeof(void_buffer)*size);

	if (!queue->buffers) {
		fprintf(stderr, "Could not initialize buffer of buffers\n");
		pthread_mutex_destroy(&queue->lock);
		pthread_cond_destroy(&queue->cond);
		return 0;
	}

	{
		int i;
		for (i=0; i<size; i++) {
			void_buffer_init(&queue->buffers[i]);
		}
	}

	// TODO: Generate a unique string if name is null
	// TODO: Search the GQL for an existing queue with the same name

	size_t len = strlen(name);
	char *copy = malloc(len+1);

	if (!copy) {
		fprintf(stderr, "Could not initialize name copy\n");
		pthread_mutex_destroy(&queue->lock);
		pthread_cond_destroy(&queue->cond);
		free(queue->buffers);
		return 0;
	}

	memcpy(copy, name, len+1);
	queue->name = copy;

	pthread_mutex_lock(&gql_lock);

	if (!gql) {
		gql_count = 4;
		gql = malloc(sizeof(void_queue*)*gql_count);
		if (!gql) abort();
		memset(gql, 0, sizeof(void_queue*)*gql_count);
	}

	// find a spot in the gql
	int found = 0;
	int i;
	for (i = 0; i < gql_count; i++) {
		if (!gql[i]) {
			found = 1;
			gql[i] = queue;
			break;
		}
	}

	if (!found) {
		gql = realloc(gql, sizeof(void_queue*)*gql_count);
		if (!gql) abort();
		memset(gql+gql_count, 0, sizeof(void_queue*)*gql_count);
		gql[gql_count] = queue;
		gql_count *= 2;
	}

	pthread_mutex_unlock(&gql_lock);

	return 1;
}

int void_queue_destroy(void_queue *queue) {
	pthread_mutex_lock(&queue->lock);
	queue->refcount--;

	if (queue->refcount == 0) {
		pthread_mutex_lock(&gql_lock);
		if (gql) {
			int i;
			for (i = 0; i < gql_count; i++) {
				if (gql[i] == queue) {
					gql[i] = NULL;
					break;
				}
			}
		}
		pthread_mutex_unlock(&gql_lock);

		int i;
		for (i = 0; i < queue->size; i++) {
			void_buffer *buffer = &queue->buffers[i];
			void_buffer_invalidate(buffer);
		}

		pthread_mutex_unlock(&queue->lock);
		pthread_mutex_destroy(&queue->lock);
		pthread_cond_destroy(&queue->cond);
		free(queue->buffers);
		free(queue->name);
		return 1;
	} else {
		pthread_mutex_unlock(&queue->lock);
		return 0;
	}
}

static int is_empty(void_queue *queue) {
	return queue->count == 0;
}

static int is_full(void_queue *queue) {
	return queue->count >= queue->size;
}

static int push(void_queue *queue, void_buffer *buffer) {
	if (is_full(queue))
		return 0;

	queue->count++;
	void_buffer_move(&queue->buffers[queue->writeIndex], buffer);
	queue->writeIndex = (queue->writeIndex+1)%queue->size;
	return 1;
}

static int pop(void_queue *queue, void_buffer *buffer) {
	if (is_empty(queue)) {
		void_buffer_invalidate(buffer);
		return 0;
	}

	queue->count--;
	void_buffer_move(buffer, &queue->buffers[queue->readIndex]);
	queue->readIndex = (queue->readIndex+1)%queue->size;
    
    pthread_cond_broadcast(&queue->cond);
    
	return 1;
}

void_queue *void_queue_get(const char *name) {
	pthread_mutex_lock(&gql_lock);
	if (gql) {
		int i;
		for (i = 0; i < gql_count; i++) {
			if (gql[i] && strcmp(gql[i]->name, name) == 0) {
				pthread_mutex_unlock(&gql_lock);
				return gql[i];
			}
		}
	}
	pthread_mutex_unlock(&gql_lock);

	return 0;
}

int void_queue_enqueue(void_queue *queue, void_buffer *buffer, int block) {
	while (block && is_full(queue)) {
		pthread_cond_wait(&queue->cond, &queue->lock);
	}

	int result = push(queue, buffer);
	pthread_cond_broadcast(&queue->cond);
	return result;
}

int void_queue_await(void_queue *queue, int64_t timeout, void_buffer *buffer) {
    struct timespec timeoutTime;
    
    if (timeout > 0) {
        clock_gettime(CLOCK_REALTIME, &timeoutTime);
        timeoutTime.tv_sec += timeout / 1000;
        timeoutTime.tv_nsec += (timeout % 1000) * 1000000;
    }
    
	while (is_empty(queue) && timeout) {
        if (timeout > 0) {
            pthread_cond_timedwait(&queue->cond, &queue->lock, &timeoutTime);
        } else {
            pthread_cond_wait(&queue->cond, &queue->lock);
        }
	}

	return pop(queue, buffer);
}

void void_queue_lock(void_queue *queue) {
	pthread_mutex_lock(&queue->lock);
}

void void_queue_unlock(void_queue *queue) {
	pthread_mutex_unlock(&queue->lock);
}
