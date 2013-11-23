
#include <stdio.h>
#include <ncurses.h>

int rows,columns; // window size

void initializations() {
	cbreak(); // cbreak so they don't have to hit enter to exit
	getmaxyx(stdscr, rows, columns); // get current window size
}

int main() {
	initscr();				/* Start curses mode 		  */
	printw("Welcome to Curses Clock\n");	/* Print welcome message	  */
	refresh();				/* Print it on to the real screen */

	initializations();
	printw("\ninitializations complete\n");
	printw("(rows=%i columns=%i)\n", rows, columns);
	refresh();				/* Print it on to the real screen */
	sleep(1);

//	getch();			/* Wait for user input */
	endwin();			/* End curses mode		  */

	return 0;
}
