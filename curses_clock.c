
#include <stdio.h>
#include <ncurses.h>

void initializations() {
	cbreak(); // cbreak so they don't have to hit enter to exit
}

int main() {
	initscr();				/* Start curses mode 		  */
	printw("Welcome to Curses Clock\n");	/* Print welcome message	  */
	refresh();				/* Print it on to the real screen */

	initializations();
	printw("initializations complete");
	refresh();				/* Print it on to the real screen */
	sleep(1);

//	getch();			/* Wait for user input */
	endwin();			/* End curses mode		  */

	return 0;
}
