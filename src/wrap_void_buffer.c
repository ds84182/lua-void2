#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <malloc.h>
#include <string.h>

#include "void_buffer.h"

#ifdef DEBUG
#define DEBUG_MSG(...) fprintf(stderr, __VA_ARGS__);
#else
#define DEBUG_MSG(...)
#endif

#define ASSERT(what, ...) if (!(what)) return luaL_error(L, __VA_ARGS__);

static int vb_create(lua_State *L) {
	size_t length = luaL_checkinteger(L, 1);
	void *data = malloc(length);

	ASSERT(data, "not enough memory for %zu byte allocation", length);

	memset(data, 0, length);

	void_buffer *buffer = lua_newuserdata(L, sizeof(void_buffer));
	void_buffer_init(buffer);
	void_buffer_set(buffer, data, length);

	DEBUG_MSG("Allocated %zu bytes for buffer %p\n", length, buffer);

	luaL_setmetatable(L, "void::buffer");

	return 1;
}

static int vb_fromString(lua_State *L) {
	luaL_checkstring(L, -1);
	size_t length;
	const char *str = lua_tolstring(L, 1, &length);

	void *data = malloc(length);

	ASSERT(data, "not enough memory for %zu byte allocation", length);

	memcpy(data, str, length);

	void_buffer *buffer = lua_newuserdata(L, sizeof(void_buffer));
	void_buffer_init(buffer);
	void_buffer_set(buffer, data, length);

	DEBUG_MSG("Allocated %zu bytes for buffer %p\n", length, buffer);

	luaL_setmetatable(L, "void::buffer");

	return 1;
}

static int vb_asString(lua_State *L) {
	void_buffer *buffer = luaL_checkudata(L, 1, "void::buffer");
	void *data = void_buffer_data(buffer);

	ASSERT(data, "no data associated with buffer %p", buffer);

	// TODO: Range (Standard Lua I J ranges with wrapping?)

	lua_pushlstring(L, (const char*)data, buffer->length);
	return 1;
}

static int vb_length(lua_State *L) {
	void_buffer *buffer = luaL_checkudata(L, 1, "void::buffer");

	lua_pushinteger(L, buffer->type == INVALID ? 0 : buffer->length);
	return 1;
}

static int vb_type(lua_State *L) {
	void_buffer *buffer = luaL_checkudata(L, 1, "void::buffer");

	switch (buffer->type) {
		case NORMAL:
			lua_pushstring(L, "buffer");
			break;
		case VIEW:
			lua_pushstring(L, "view");
			break;
		case INVALID:
			lua_pushstring(L, "invalid");
			break;
		default:
			lua_pushstring(L, "unknown");
	}
	return 1;
}

// void.buffer.clone(buffer, [i, [j]])
static int vb_clone(lua_State *L) {
	void_buffer *buffer = luaL_checkudata(L, 1, "void::buffer");
	ptrdiff_t i = luaL_optinteger(L, 2, 0);
	ptrdiff_t j = luaL_optinteger(L, 3, -1);

	void *data = void_buffer_data(buffer);

	ASSERT(data, "no data associated with buffer %p", buffer);

	// Lua style range handling
	if (i < 0) i = buffer->length+i;
	if (i <= 0) i = 0;
	if (i >= buffer->length) i = buffer->length-1;
	i--;

	if (j < 0) j = buffer->length+j;
	if (j <= 0) j = 0;
	if (j >= buffer->length) j = buffer->length-1;
	j--;

	// TODO: Should we output preprocessed too?
	ASSERT(j >= i, "invalid range for clone (%d %d)", i, j)

	size_t rangeSize = (j-i)+1;
	void *newData = malloc(rangeSize);

	ASSERT(newData, "not enough memory for %zu byte allocation", rangeSize);

	memcpy(newData, data+i, rangeSize);

	void_buffer *newBuffer = lua_newuserdata(L, sizeof(void_buffer));
	void_buffer_init(newBuffer);
	void_buffer_set(newBuffer, newData, rangeSize);

	DEBUG_MSG("Allocated %zu bytes for buffer %p\n", rangeSize, newBuffer);

	luaL_setmetatable(L, "void::buffer");

	return 1;
}

// void.buffer.concat(buffer1, buffer2, buffer3... buffern)
static int vb_concat(lua_State *L) {
	int nargs = lua_gettop(L);

	size_t size;
	void *data;

	if (nargs == 0) {
		size = 0;
		data = malloc(size);

		ASSERT(data, "not enough memory for %zu byte allocation", size);
	} else if (nargs == 1) {
		// copy buffer
		void_buffer *buffer = luaL_checkudata(L, 1, "void::buffer");
		size = buffer->length;

		void *bufdata = void_buffer_data(buffer);

		ASSERT(bufdata, "no data associated with buffer %p", buffer);

		data = malloc(size);
		ASSERT(data, "not enough memory for %zu byte allocation", size);
		memcpy(data, bufdata, size);
	} else {
		size = 0;

		int i;
		for (i=0; i<nargs; i++) {
			// Check types and tally up the buffer length
			void_buffer *buffer = luaL_checkudata(L, i+1, "void::buffer");
			ASSERT(void_buffer_data(buffer), "no data associated with buffer %p", buffer)
			size += buffer->length;
		}

		data = malloc(size);
		ASSERT(data, "not enough memory for %zu byte allocation", size);

		void *dataptr = data;
		for (i=0; i<nargs; i++) {
			// Copy data
			void_buffer *buffer = lua_touserdata(L, i+1);

			void *bufdata = void_buffer_data(buffer);
			memcpy(dataptr, bufdata, buffer->length);
			dataptr += buffer->length;
		}
	}

	void_buffer *buffer = lua_newuserdata(L, sizeof(void_buffer));
	void_buffer_init(buffer);
	void_buffer_set(buffer, data, size);

	DEBUG_MSG("Allocated %zu bytes for buffer %p\n", size, buffer);

	luaL_setmetatable(L, "void::buffer");

	return 1;
}

// TODO: Lua like range calculation (think string.sub)
static int vb_view(lua_State *L) {
	void_buffer *source = luaL_checkudata(L, 1, "void::buffer");
	ptrdiff_t offset = luaL_checkinteger(L, 2);
	size_t size = luaL_checkinteger(L, 3);

	void_buffer *buffer = lua_newuserdata(L, sizeof(void_buffer));
	void_buffer_init(buffer);
	int err = void_buffer_view(buffer, source, offset, size);

	if (err == VOID_EOUTOFRANGE) {
		return luaL_error(L, "offset+size out of range (offset: %d, size: %zu)", offset, size);
	} else if (err != VOID_SUCCESS) {
		return luaL_error(L, "generic void_buffer_view error %d", err);
	}

	luaL_setmetatable(L, "void::buffer");
	return 1;
}

static int vb_invalidate(lua_State *L) {
	void_buffer *buffer = luaL_checkudata(L, -1, "void::buffer");
	void_buffer_invalidate(buffer);
	return 0;
}

static int vb_grow(lua_State *L) {
	void_buffer *buffer = luaL_checkudata(L, 1, "void::buffer");
	size_t size = luaL_checkinteger(L, 2);
    
    int err = void_buffer_grow(buffer, size);
    return 0;
}

static int vb_shrink(lua_State *L) {
	void_buffer *buffer = luaL_checkudata(L, 1, "void::buffer");
	size_t size = luaL_checkinteger(L, 2);
    
    int err = void_buffer_shrink(buffer, size);
    return 0;
}

// TODO: vb_grow and vb_shrink

#define BUFFER_GETTER(type,reversed,luatype,name) static int vb_get ## name (lua_State *L) { \
	void_buffer *buffer = luaL_checkudata(L, 1, "void::buffer"); \
	unsigned char *data = void_buffer_data(buffer); \
	ASSERT(data, "no data associated with buffer %p", buffer) \
	\
	ptrdiff_t offset = luaL_checkinteger(L, 2); \
	ASSERT(offset >= 0 && offset+sizeof(type) <= buffer->length, "offset %d out of range", offset) \
	\
	type value; \
	unsigned char *valuePtr = (unsigned char*) &value; \
	if (reversed) { \
		unsigned char *dataOffset = data+offset+sizeof(type); \
		size_t size = sizeof(type); \
		while (size) { \
			*(valuePtr++) = *(--dataOffset); \
			size--; \
		} \
	} else { \
		unsigned char *dataOffset = data+offset; \
		size_t size = sizeof(type); \
		while (size) { \
			*(valuePtr++) = *(dataOffset++); \
			size--; \
		} \
	} \
	lua_push ## luatype (L, value); \
	return 1; \
}

#define BUFFER_SETTER(type,reversed,luatype,name) static int vb_set ## name (lua_State *L) { \
	void_buffer *buffer = luaL_checkudata(L, 1, "void::buffer"); \
	unsigned char *data = void_buffer_data(buffer); \
	ASSERT(data, "no data associated with buffer %p", buffer) \
	\
	ptrdiff_t offset = luaL_checkinteger(L, 2); \
	ASSERT(offset >= 0 && offset+sizeof(type) <= buffer->length, "offset %d out of range", offset) \
	\
	type value = luaL_check ## luatype (L, 3); \
	unsigned char *valuePtr = (unsigned char*) &value; \
	if (reversed) { \
		unsigned char *dataOffset = data+offset+sizeof(type); \
		size_t size = sizeof(type); \
		while (size) { \
			*(--dataOffset) = *(valuePtr++); \
			size--; \
		} \
	} else { \
		unsigned char *dataOffset = data+offset; \
		size_t size = sizeof(type); \
		while (size) { \
			*(dataOffset++) = *(valuePtr++); \
			size--; \
		} \
	} \
	return 0; \
}

#define BUFFER_GETTER_SETTER(type,reversed,luatype,name) BUFFER_GETTER(type,reversed,luatype,name) \
BUFFER_SETTER(type,reversed,luatype,name)

BUFFER_GETTER_SETTER(uint8_t,0,integer,U8)
BUFFER_GETTER_SETTER(int8_t,0,integer,S8)

// Native Endian
BUFFER_GETTER_SETTER(uint16_t,0,integer,U16)
BUFFER_GETTER_SETTER(int16_t,0,integer,S16)
BUFFER_GETTER_SETTER(uint32_t,0,integer,U32)
BUFFER_GETTER_SETTER(int32_t,0,integer,S32)
BUFFER_GETTER_SETTER(uint64_t,0,integer,U64)
BUFFER_GETTER_SETTER(int64_t,0,integer,S64)
BUFFER_GETTER_SETTER(float,0,number,F32)
BUFFER_GETTER_SETTER(double,0,number,F64)

// Reverse Native Endian
BUFFER_GETTER_SETTER(uint16_t,1,integer,U16RE)
BUFFER_GETTER_SETTER(int16_t,1,integer,S16RE)
BUFFER_GETTER_SETTER(uint32_t,1,integer,U32RE)
BUFFER_GETTER_SETTER(int32_t,1,integer,S32RE)
BUFFER_GETTER_SETTER(uint64_t,1,integer,U64RE)
BUFFER_GETTER_SETTER(int64_t,1,integer,S64RE)
BUFFER_GETTER_SETTER(float,1,number,F32RE)
BUFFER_GETTER_SETTER(double,1,number,F64RE)

#define DEF(name) {"get" #name, vb_get ## name}, {"set" #name, vb_set ## name}
#if __BYTE_ORDER__ ==  __ORDER_LITTLE_ENDIAN__
#define DEF_LE(name) {"get" #name "LE", vb_get ## name}, {"set" #name "LE", vb_set ## name}
#define DEF_BE(name) {"get" #name "BE", vb_get ## name ## RE}, {"set" #name "BE", vb_set ## name ## RE}
#else
#define DEF_LE(name) {"get" #name "LE", vb_get ## name ## RE}, {"set" #name "LE", vb_set ## name ## RE}
#define DEF_BE(name) {"get" #name "BE", vb_get ## name}, {"set" #name "BE", vb_set ## name}
#endif

static const luaL_Reg library[] = {
	{"create", vb_create},
	{"fromString", vb_fromString},
	{"asString", vb_asString},
	{"length", vb_length},
	{"type", vb_type},
	{"clone", vb_clone},
	{"concat", vb_concat},
	{"view", vb_view},
	{"invalidate", vb_invalidate},
    {"grow", vb_grow},
    {"shrink", vb_shrink},

	DEF(U8),
	DEF(S8),

	DEF(U16),
	DEF_LE(U16),
	DEF_BE(U16),
	DEF(S16),
	DEF_LE(S16),
	DEF_BE(S16),

	DEF(U32),
	DEF_LE(U32),
	DEF_BE(U32),
	DEF(S32),
	DEF_LE(S32),
	DEF_BE(S32),

	DEF(U64),
	DEF_LE(U64),
	DEF_BE(U64),
	DEF(S64),
	DEF_LE(S64),
	DEF_BE(S64),

	DEF(F32),
	DEF_LE(F32),
	DEF_BE(F32),
	DEF(F32),
	DEF_LE(F32),
	DEF_BE(F32),

	DEF(F64),
	DEF_LE(F64),
	DEF_BE(F64),
	DEF(F64),
	DEF_LE(F64),
	DEF_BE(F64),
	{NULL, NULL}
};

static const luaL_Reg metatable[] = {
	{"__gc", vb_invalidate},
	{"__len", vb_length},
	{"__index", vb_getU8},
	{"__newindex", vb_setU8},
	// TODO: add could return a view... maybe
	{NULL, NULL}
};

static void vb_make_metatable(lua_State *L) {
	luaL_newmetatable(L, "void::buffer");
	// void::buffer:metatable
	luaL_setfuncs(L, metatable, 0);
	// void::buffer:metatable
	lua_pop(L, 1);
	// nothing
}

int lvoid_buffer_open(lua_State *L) {
	vb_make_metatable(L);
	luaL_newlib(L, library);
	// void.buffer:table

	return 1;
	// void.buffer:table
}
