#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "void_queue.h"

#ifdef DEBUG
#define DEBUG_MSG(...) fprintf(stderr, __VA_ARGS__);
#else
#define DEBUG_MSG(...)
#endif

#define ASSERT(what, ...) if (!(what)) return luaL_error(L, __VA_ARGS__);

bool xorshiftinit = false;
uint64_t xorshiftSeed[2];

uint64_t xorshift128plus() {
	uint64_t s1 = xorshiftSeed[0];
	const uint64_t s0 = xorshiftSeed[1];
	xorshiftSeed[0] = s0;
	s1 ^= s1 << 23;
	return ( xorshiftSeed[1] = ( s1 ^ s0 ^ ( s1 >> 17 ) ^ ( s0 >> 26 ) ) ) + s0;
}

static int vq_create(lua_State *L) {
    if (!xorshiftinit) {
        xorshiftinit = true;
        // Fuck it, it's random enough
        uint64_t randomStackValues[4];
        time_t currentTime = time(NULL);
        xorshiftSeed[0] = ((randomStackValues[0] << 24) +
            (randomStackValues[2] << 32) +
            (((uint64_t)&randomStackValues) >> 16)) + currentTime;
        xorshiftSeed[1] = ((randomStackValues[1] << 24) +
            (randomStackValues[3] << 32) +
            (((uint64_t)&randomStackValues) >> 8)) + (currentTime << 24);
        xorshift128plus(); xorshift128plus(); // Randomize and shift the seeds
    }

	unsigned int size = luaL_checkinteger(L, 1);
	const char *name = luaL_optstring(L, 2, NULL);
    char fmtname[512];

	void_queue *queue = malloc(sizeof(void_queue));

	ASSERT(queue, "not enough memory to allocate queue object");
    
    if (name == NULL) {
        snprintf(fmtname, 512, "%016lX", xorshift128plus()+((uint64_t)L)+((uint64_t)queue));
        name = fmtname;
    }

	if (!void_queue_init(queue, size, name)) {
		free(queue);
		ASSERT(false, "not enough memory to allocate queue data");
	}

	void_queue **queueHolder = lua_newuserdata(L, sizeof(void_queue*));
	*queueHolder = queue;

	luaL_setmetatable(L, "void::queue");
    lua_pushstring(L, name);

	return 2;
}

static int vq_destroy(lua_State *L) {
	void_queue **queueHolder = luaL_checkudata(L, 1, "void::queue");

	if (*queueHolder) {
		if (void_queue_destroy(*queueHolder)) {
			// TODO: This probably needs to use a destructor callback (This could not be allocated using malloc)
			free(*queueHolder);
		}

		*queueHolder = 0;
	}

	return 0;
}

static int vq_get(lua_State *L) {
	const char *name = luaL_checkstring(L, 1);

	void_queue *queue = void_queue_get(name);

	if (!queue) {
		lua_pushnil(L);
	} else {
		// TODO: Maybe have a registry table that holds refs to
		// void_queue holders? (so we don't have to make multiple)
		ASSERT(queue->refcount > 0, "What");
		queue->refcount++;
		void_queue **queueHolder = lua_newuserdata(L, sizeof(void_queue*));
		*queueHolder = queue;

		luaL_setmetatable(L, "void::queue");
	}

	return 1;
}

static int luaL_optboolean(lua_State *L, int narg, int def) {
	return lua_isboolean(L, narg) ? lua_toboolean(L, narg) : def;
}

static int vq_enqueue(lua_State *L) {
	void_queue **queueHolder = luaL_checkudata(L, 1, "void::queue");
	void_queue *queue = *queueHolder;
	void_buffer *buffer = luaL_checkudata(L, 2, "void::buffer");
	int block = luaL_optboolean(L, 3, 0);

	// Make sure the buffer is valid...
	void *data = void_buffer_data(buffer);
	ASSERT(data, "no data associated with buffer %p", buffer);

	// Now lock the queue
	void_queue_lock(queue);

	if (queue->count >= queue->size && !block) {
		// Queue is full and we aren't blocking... No reason to
		// call void_queue_enqueue and do a whole bunch of
		// buffer stuff
		lua_pushboolean(L, 0);
	} else {
		if (buffer->type == VIEW) {
			// We need to make a new buffer from this view
			void_queue_unlock(queue);
			ASSERT(false, "enqueueing views is currently not supported");
		}

		lua_pushboolean(L, void_queue_enqueue(queue, buffer, block));
	}

	void_queue_unlock(queue);

	return 1;
}

static int vq_await(lua_State *L) {
	void_queue **queueHolder = luaL_checkudata(L, 1, "void::queue");
	void_queue *queue = *queueHolder;
	int64_t timeout = luaL_optinteger(L, 2, 0);
    void_buffer *buffer = lua_isnoneornil(L, 3) ? NULL : luaL_checkudata(L, 3, "void::buffer");

	// Now lock the queue
	void_queue_lock(queue);

    if (!buffer) {
        buffer = lua_newuserdata(L, sizeof(void_buffer));
        void_buffer_init(buffer);
        luaL_setmetatable(L, "void::buffer");
    } else {
        lua_pushvalue(L, 3);
    }

	void_queue_await(queue, timeout, buffer);

	void_queue_unlock(queue);

	return 1;
}

static int vq_count(lua_State *L) {
	void_queue **queueHolder = luaL_checkudata(L, 1, "void::queue");
	void_queue *queue = *queueHolder;

	void_queue_lock(queue);
	lua_pushinteger(L, queue->count);
	void_queue_unlock(queue);

	return 1;
}

static const luaL_Reg library[] = {
	{"create", vq_create},
	{"destroy", vq_destroy},
	{"get", vq_get},
	{"enqueue", vq_enqueue},
	{"await", vq_await},
	{"count", vq_count},
	{NULL, NULL}
};

static const luaL_Reg metatable[] = {
	{"__gc", vq_destroy},
	{NULL, NULL}
};

static void vq_make_metatable(lua_State *L) {
	luaL_newmetatable(L, "void::queue");
	// void::queue:metatable
	luaL_setfuncs(L, metatable, 0);
	// void::queue:metatable
	lua_pop(L, 1);
	// nothing
}

int lvoid_queue_open(lua_State *L) {
	vq_make_metatable(L);
	luaL_newlib(L, library);
	// void.queue:table

	return 1;
	// void.queue:table
}
