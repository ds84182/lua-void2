#include "void_buffer.h"

#include <string.h>
#include <malloc.h>

// Initializes a buffer as an invalid buffer
void void_buffer_init(void_buffer *buffer) {
	memset(buffer, 0, sizeof(void_buffer));
	buffer->type = INVALID;
}

static void attach(void_buffer *buffer, void_buffer *to) {
	if (!to->normal.attached) {
		to->normal.attached = malloc(sizeof(void_buffer*)*4);
		memset(to->normal.attached, 0, sizeof(void_buffer*)*4);
		to->normal.attachLength = 4;
	}

	int i;
	for (i=0; i<to->normal.attachLength; i++) {
		if (!to->normal.attached[i]) {
			to->normal.attached[i] = buffer;
			buffer->view.attachPoint = i;
			return;
		}
	}

	// Reallocate attach buffer with double the space
	// TODO: Make sure size does not overflow
	// TODO: Make sure that realloc and malloc don't error
	int oldLength = to->normal.attachLength;
	to->normal.attachLength *= 2;
	to->normal.attached = realloc(to->normal.attached, sizeof(void_buffer*)*to->normal.attachLength);
	memset(to->normal.attached+oldLength, 0, sizeof(void_buffer*)*oldLength);
	to->normal.attached[oldLength] = buffer;
	buffer->view.attachPoint = oldLength;
}

static void freeData(void_buffer *buffer) {
	if (buffer->type == NORMAL && buffer->normal.data) {
        free(buffer->normal.data);
		buffer->normal.data = 0;
	}
}

// TODO: Use reference counting with normal buffers so a buffer
// going out of scope or getting GC'd won't clean up it's data
// until all it's views die
// This can be done by making a void_buffer_data object that has
// that reference count.
// Views contain a pointer to the buffer data object instead of to
// the parent buffer
// Attach detach logic would still have to be tracked in buffer data
// So that doing a void_buffer_move would detach all attached buffers
static void detachAll(void_buffer *buffer) {
	if (buffer->type == NORMAL && buffer->normal.attached && buffer->normal.attachLength) {
		int i;
		for (i=0; i<buffer->normal.attachLength; i++) {
			void_buffer *attached = buffer->normal.attached[i];
			if (attached) {
				void_buffer_invalidate(attached);
			}
		}

		free(buffer->normal.attached);
		buffer->normal.attachLength = 0;
	}
}

static void detach(void_buffer *buffer) {
	if (buffer->type == VIEW && buffer->view.buffer) {
		buffer->view.buffer->normal.attached[buffer->view.attachPoint] = 0;
	}
}

// Sets the data in the buffer and changes the type to normal
void void_buffer_set(void_buffer *buffer, void *data, size_t length) {
	void_buffer_invalidate(buffer);
	buffer->type = NORMAL;
	buffer->length = length;
	buffer->normal.data = data;
}

// Creates a view of the buffer and attaches itself to the parent buffer
// If buffer is a view of another buffer, it attaches to the parent buffer
int void_buffer_view(void_buffer *buffer, void_buffer *of, ptrdiff_t start, size_t length) {
	void_buffer_invalidate(buffer);

	if (start < 0)
		return VOID_EOUTOFRANGE;

	while (of->type == VIEW) {
		if (start+length > of->view.start+of->length) {
			return VOID_EOUTOFRANGE;
		} else {
			start = start+of->view.start;
			of = of->view.buffer;
		}
	}

	if (start+length > of->length) {
		return VOID_EOUTOFRANGE;
	}

	buffer->type = VIEW;
	buffer->length = length;
	buffer->view.start = start;
	buffer->view.buffer = of;
	attach(buffer, of);

	return VOID_SUCCESS;
}

// Marks a buffer as invalid, removes pointers, and detaches attached views
void void_buffer_invalidate(void_buffer *buffer) {
	detachAll(buffer);
	freeData(buffer);
	detach(buffer);

	buffer->type = INVALID;
}

// Copies the data from one buffer object to another
void void_buffer_copy(void_buffer *dest, const void_buffer *source) {
	memcpy(dest, source, sizeof(void_buffer));

	if (dest->type == VIEW) {
		attach(dest, source->view.buffer);
	}
}

int void_buffer_move(void_buffer *dest, void_buffer *source) {
	if (source->type == NORMAL) {
		detachAll(dest); // Detach all views
		// memmove will work
		memcpy(dest, source, sizeof(void_buffer));
		// zero out the source
		void_buffer_init(source);

		return VOID_SUCCESS;
	} else if (source->type == VIEW) {
		// We need to copy the data into a new buffer
		// TODO: This is not within the scope of this file.
			// This file does not do buffer allocation management
	}

	return VOID_EWRONGTYPE;
}

void *void_buffer_data(const void_buffer *buffer) {
	if (buffer->type == NORMAL) {
		return buffer->normal.data;
	} else if (buffer->type == VIEW) {
		return (buffer->view.buffer->normal.data)+buffer->view.start;
	} else {
		return 0;
	}
}

int void_buffer_grow(void_buffer *buffer, size_t newLength) {
    if (buffer->type == NORMAL) {
        if (buffer->length < newLength) {
            buffer->normal.data = realloc(buffer->normal.data, newLength);
            buffer->length = newLength;
        }
        return 0;
    } else {
        return VOID_EWRONGTYPE;
    }
}

int void_buffer_shrink(void_buffer *buffer, size_t newLength) {
    if (buffer->type == NORMAL) {
        if (buffer->length > newLength) {
            buffer->normal.data = realloc(buffer->normal.data, newLength);
            buffer->length = newLength;
        }
        return 0;
    } else {
        return VOID_EWRONGTYPE;
    }
}
