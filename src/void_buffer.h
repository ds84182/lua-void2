#ifndef VOID_BUFFER_H
#define VOID_BUFFER_H

#include <stddef.h>

#define VOID_SUCCESS 0
#define VOID_EOUTOFRANGE -1
#define VOID_EWRONGTYPE -2

enum void_buffer_type {
	NORMAL,
	VIEW,
	INVALID
};

typedef struct void_buffer void_buffer;

struct void_buffer {
	int type;
	size_t length;
	union {
		struct {
			void *data;
			size_t attachLength;
			void_buffer **attached;
		} normal;

		struct {
			ptrdiff_t start;
			void_buffer *buffer;
			int attachPoint;
		} view;

		/*struct {

		} invalid;*/
	};
};

// Initializes a buffer as an invalid buffer
void void_buffer_init(void_buffer *buffer);
// Sets the data in the buffer and changes the type to normal
void void_buffer_set(void_buffer *buffer, void *data, size_t length);
// Creates a view of the buffer and attaches itself to the parent buffer
// If buffer is a view of another buffer, it attaches to the parent buffer
int void_buffer_view(void_buffer *buffer, void_buffer *of, ptrdiff_t start, size_t length);
// Marks a buffer as invalid, removes pointers, and detaches attached views
void void_buffer_invalidate(void_buffer *buffer);
// Copies the data from one buffer object to another
void void_buffer_copy(void_buffer *dest, const void_buffer *source);
// Moves a buffer from one buffer instance to another
int void_buffer_move(void_buffer *dest, void_buffer *source);
// Returns a pointer to the buffer's data or null if no data is attached
void *void_buffer_data(const void_buffer *buffer);
int void_buffer_grow(void_buffer *buffer, size_t newLength);
int void_buffer_shrink(void_buffer *buffer, size_t newLength);

#endif
