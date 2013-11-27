
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <string.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>

int rows,columns; // window size
gchar *font;

static const gchar *default_config = "\
{\
	\"timezones\": [\
		\"\",\
                \"America/New_York\",\
                \"GMT\",\
                \"Europe/Paris\",\
                \"Asia/Baghdad\",\
                \"Asia/Singapore\",\
                \"Asia/Tokyo\"\
	],\
	\"font\": \"Lat2-VGA8.psf.gz\",\
	\"fontpath\": [\
		\"/usr/share/consolefonts\"\
	]\
}\
";

void initializations() {
	JsonParser *parser;
	JsonNode *root;
	JsonObject *jsonObj;
	GError *error;

	// parse JSON
#if !defined(GLIB_VERSION_2_36) 
	g_type_init(); 
#endif 
	parser = json_parser_new ();

	error = NULL;
	json_parser_load_from_data (parser, default_config, strlen(default_config), &error);
	if (error) {
		endwin();
		g_print ("Unable to parse config: %s\n", error->message);
		g_print("%s\n",default_config);
		g_error_free (error);
		g_object_unref (parser);
		exit(EXIT_FAILURE);
	}

	root = json_parser_get_root (parser);
	jsonObj = json_node_get_object(root);

	// get font
	font = json_object_get_string_member(jsonObj,"font");
	printw("font=%s\n",font);

	// get fontpath

	// does font exist?

	// clean up
	g_object_unref (parser);

//	cbreak(); // cbreak so they don't have to hit enter to exit
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

	printf("exiting because the user hit a key\n");

	return 0;
}
