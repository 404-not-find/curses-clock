LIBS=-lncurses
GLIB_INCLUDES:=$(shell pkg-config --cflags glib-2.0)
INCLUDES=-I/usr/include/json-glib-1.0 ${GLIB_INCLUDES}

cclock: curses_clock.c
	gcc -o cclock curses_clock.c ${LIBS} ${INCLUDES}
