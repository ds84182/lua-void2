T = void_core

CONFIG = ./config

include $(CONFIG)

SRCS = src/thread_compat.c src/void_buffer.c src/void_queue.c src/wrap_void.c src/wrap_void_buffer.c src/wrap_void_queue.c
OBJS = src/thread_compat.o src/void_buffer.o src/void_queue.o src/wrap_void.o src/wrap_void_buffer.o src/wrap_void_queue.o

lib: src/void_core.so

src/void_core.so: $(OBJS)
	$(CC) $(CFLAGS) $(LIB_OPTION) -o src/void_core.so $(OBJS)

test: lib
	cd tests; lua5.3 test.lua

test-gdb: lib
	cd tests; gdb --args lua5.3 test.lua

install:
	mkdir -p $(INST_LIBDIR)/void
	mkdir -p $(INST_LUADIR)/void
	install -m 644 src/void_core.so $(INST_LIBDIR)/void/core.so
	cp -p lua/void/*.lua $(INST_LUADIR)/void
	cp -p lua/void.lua $(INST_LUADIR)

clean:
	rm -f src/void_core.so $(OBJS)
