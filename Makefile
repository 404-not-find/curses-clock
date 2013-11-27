LIBS=-lncurses 
GLIB_INCLUDES:=$(shell pkg-config --libs --cflags glib-2.0)
GLIB_JSON_INCLUDES:=$(shell pkg-config --libs --cflags json-glib-1.0)
INCLUDES=${GLIB_INCLUDES} ${GLIB_JSON_INCLUDES}

cclock: curses_clock.c
	gcc -o cclock curses_clock.c ${LIBS} ${INCLUDES} -std=gnu99
