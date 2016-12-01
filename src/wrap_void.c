#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

extern int lvoid_buffer_open(lua_State *L);
extern int lvoid_queue_open(lua_State *L);

int luaopen_void_core(lua_State *L) {
	lua_createtable(L, 0, 3);
	// void:table

	lvoid_buffer_open(L);
	// void.buffer:table void:table
	lua_setfield(L, -2, "buffer");
	// void:table

	lvoid_queue_open(L);
	// void.queue:table void:table
	lua_setfield(L, -2, "queue");
	// void:table

	return 1;
	// void:table
}
