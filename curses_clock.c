
#include <stdio.h>
#include <ncurses.h>
#include <time.h>
#include <string.h>

int rows,columns; // window size

void initializations() {
	cbreak(); // cbreak so they don't have to hit enter to exit
	getmaxyx(stdscr, rows, columns); // get current window size
	nodelay(stdscr, TRUE); // read characters in non-blocking mode
}

int kbhit () {
	int ch = getch();

	if (ch != ERR) {
		ungetch(ch);
		return(1);
	} else {
		return(0);
	}
}

void display_clock() {
	int centerx,centery;
	char* time_string;
	time_t now;
	size_t width;
	while (1) {
		clear();
		centerx = rows/2;
		centery = columns/2;

		now=time(NULL);
		time_string=ctime(&now);
		width = strlen(time_string);

		mvprintw(centerx,centery-(width/2),"%s",time_string);
		refresh();	// Print it on to the real screen

		if (kbhit()) {
			return;
		}

		getmaxyx(stdscr, rows, columns); // get current window size
		sleep(1);
	}
}

int main() {
	initscr();				// Start curses mode
	printw("Welcome to Curses Clock\n");	// Print welcome message
	refresh();				// Print it on to the real screen

	initializations();
	printw("\ninitializations complete\n");
	printw("(rows=%i columns=%i)\n", rows, columns);
	refresh();	// Print it on to the real screen
	sleep(1);
	
	display_clock(); // show the clock

	endwin();	// End curses mode

	return 0;
}
