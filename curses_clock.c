
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ncurses.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <glib-unix.h>
#include <json-glib/json-glib.h>

int rows,columns; // window size
const gchar *font;

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
		\"/usr/share/consolefonts\",\
		\"/home/chicks/consolefonts\"\
	]\
}\
";


int file_exists (char * fileName) {
	struct stat buf;
	int i = stat ( fileName, &buf );
	/* File found */
	if ( i == 0 ) {
		return 1;
	}
	return 0;
}

int read_font (const char * filename) {
	char buffer[10];
	char font[128*1024]; // the biggest I've seen in the wild is <30k
	gint pipefds[2];
	int compressed = 0;
	size_t got;

	FILE * fh = g_fopen(filename,"r");

	fread(&buffer,2,1,fh);

//	printf("%x %x\n",buffer[0],buffer[1] & 0xff);
	if ((buffer[0] == 0x1f) && ((buffer[1] & 0xff) == 0x8b)) {
		char *base_cmd = "/bin/gzip -dc ";
		compressed++;
//		printf("font %s is compressed\n",filename);
		fclose(fh);

		gchar * cmd = g_strconcat(base_cmd,filename,NULL);
		fh = popen(cmd,"r");
		if (fh == NULL) {
			printf("ERROR: gzip pipe open failed: %s",strerror(errno));
			exit(1);
		}
	} else {
		printf("font %s is NOT compressed\n",filename);
		// TODO: rewind fh
		exit(3);
	}

	got = fread(&font,1,sizeof(font),fh);

	printf("font %s is %i bytes uncompressed\n",filename,(int) got);

	if (got == sizeof(font)) {
		printf("ERROR: font %s is bigger than our buffer can handle, exiting.\n",filename);
		exit(2);
	}

	// determine PSF version
	int glyphs, height, width, char_size, bytes_per_row;
	if (
		((font[0] & 0xff) == 0x36)
		&& ((font[1] & 0xff) == 0x04)
	) {
		// v1
		width = 8;
		bytes_per_row = 1;
		height = char_size = (font[3] & 0x99);
		int mode = (font[2] & 0x99);
		if ( mode & 0b001 ) {
			glyphs=512;
		} else {
			glyphs=256;
		}
		printf("FONT[0-4]: x%02x x%02x x%02x x%02x\n",font[0] & 0xff, font[1] & 0xff, font[2] & 0xff, font[3] & 0xff );
		printf("FONT: width=%i height=%i bytes_per_row=%i glyphs=%i\n", width, height, bytes_per_row, glyphs );
	} else if (
		((font[0] & 0xff) == 0x72)
		&& ((font[1] & 0xff) == 0x04)
		&& ((font[2] & 0xff) == 0x4a)
		&& ((font[3] & 0xff) == 0x86)
	) {
		// v2
		printf("ERROR: font %s is a v2 PSF which is UNIMPLEMENTED, exiting.\n",filename);
		// TODO: implement!!!
		exit(4);
	} else {
		// wtf?
		printf("FONT[0-4]: x%02x x%02x x%02x x%02x\n",font[0] & 0xff, font[1] & 0xff, font[2] & 0xff, font[3] & 0xff );
		printf("ERROR: font %s is a not a recognized PSF version, exiting.\n",filename);
		exit(4);
	}

	// clean up fh
	if (compressed) {
		pclose(fh);
	} else {
		fclose(fh);
	}
	exit(42); // TODO: remove after read_font() debugged

	return 0;
}

void initializations() {
	JsonParser *parser;
	JsonNode *root;
	JsonObject *jsonObj;
	GError *error;
	JsonArray *fontpath;

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
//	printf("font=%s\n",font);

	// get fontpath
	fontpath = json_object_get_array_member(jsonObj,"fontpath");

	// does font exist?
	for (guint index = 0; index < json_array_get_length(fontpath); index++) {
		const gchar *thispath;
		thispath = json_array_get_string_element(fontpath, index);
//		printf("thispath=%s\n",thispath);
		gchar * check_file = g_strconcat(thispath,"/",font,NULL);
		if ( file_exists(check_file) ) {
			font = check_file;
			printf("found font=%s\n",font);
		}
	}

	// read font in
	read_font(font);

	// handy for debugging
	refresh();	// Print it on to the real screen
	sleep(3);

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
	printf("Welcome to Curses Clock\n");	// Print welcome message

	initializations();
	initscr();	// Start curses mode
	printw("\ninitializations complete\n");
	printw("(rows=%i columns=%i)\n", rows, columns);
	refresh();	// Print it on to the real screen
	sleep(1);
	
	display_clock(); // show the clock

	endwin();	// End curses mode

	printf("exiting because the user hit a key\n");

	return 0;
}
