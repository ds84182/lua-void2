#ifndef VOID_QUEUE
#define VOID_QUEUE

#include "thread_compat.h"
#include "void_buffer.h"

#include <stdint.h>

typedef struct void_queue void_queue;

struct void_queue {
	pthread_mutex_t lock;
	pthread_cond_t cond;
	unsigned int refcount;
	// This buffer is a circular buffer
	// bufferQueueIndex wraps around
	unsigned int size;
	unsigned int count;
	unsigned int readIndex;
	unsigned int writeIndex;
	void_buffer *buffers;
	char *name;
};

int void_queue_init(void_queue *queue, unsigned int size, const char *name);
int void_queue_destroy(void_queue *queue);

// Locks globally then checks a dynamically allocated list to see
// if a void_queue exists with that name.
// If a void_queue does not exists, this returns 0
void_queue *void_queue_get(const char *name);

// This will create a new buffer object to hold this buffer's data
// and will invalidate the current buffer
// If not blocking, it returns 1 if the buffer was successfully
// inserted, and 0 if not
// If blocking, it will return 1 if successful
// If an error occurs, the return value will be negative
// Errors:
	// ENODATA - Buffer has no data attached to it
	// ENOMEM - Not enough memory
	// ELOCKFAIL - Lock operation failed
int void_queue_enqueue(void_queue *queue, void_buffer *buffer, int block);

// Waits for a queue to have a buffer available
// If timeout milliseconds pass, this method will return null
// If an error occurs, this method will return null
// TODO: Maybe better error codes?
int void_queue_await(void_queue *queue, int64_t timeout, void_buffer *buffer);

void void_queue_lock(void_queue *queue);
void void_queue_unlock(void_queue *queue);

#endif
