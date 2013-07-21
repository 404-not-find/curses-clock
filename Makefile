LIBS=-lncurses

cclock: curses_clock.c
	gcc -o cclock curses_clock.c ${LIBS}
