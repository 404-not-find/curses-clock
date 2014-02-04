
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ncurses.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <glib-unix.h>
#include <json-glib/json-glib.h>

//
// global variables
//
int rows,columns; // window size

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
	\"font\": \"Lat7-Terminus32x16.psf.gz\",\
	\"fontpath\": [\
		\"/usr/share/consolefonts\",\
		\"/home/chicks/consolefonts\"\
	]\
}\
";

struct Font {
	int glyphs, height, width, char_size, bytes_per_row, font_start;
	union {
		char data[128*1024]; // the biggest I've seen in the wild is <30k
		struct Header {
			guint32 magic;
			guint32 version;
			guint32 header_size;
			guint32 flags;
			guint32 glyphs;
			guint32 char_size;
			guint32 height;
			guint32 width;
		} header;
	};
	unsigned int charmap[512];
};

struct Font myfont;

gchar *timezones[20];

/********************************
 *        functions
 */

int file_exists (char * fileName) {
	struct stat buf;
	int i = stat ( fileName, &buf );
	/* File found */
	if ( i == 0 ) {
		return 1;
	}
	return 0;
}

const char *byte_to_binary(int x) {
    static char b[9];
    b[0] = '\0';

    for (int z = 128; z > 0; z >>= 1) {
        strcat(b, ((x & z) == z) ? "X" : ".");
    }

    return b;
}

int read_font (const char * filename) {
	char buffer[10];
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

	got = fread(&myfont.data,1,sizeof(myfont.data),fh);

	printf("font %s is %i bytes uncompressed\n",filename,(int) got);

	if (got == sizeof(myfont.data)) {
		printf("ERROR: font %s is bigger than our buffer can handle, exiting.\n",filename);
		exit(2);
	}

	// determine PSF version
	if (
		((myfont.data[0] & 0xff) == 0x36)
		&& ((myfont.data[1] & 0xff) == 0x04)
	) {
		// v1
		myfont.width = 8;
		myfont.bytes_per_row = 1;
		myfont.font_start=4;
		myfont.height = myfont.char_size = (myfont.data[3] & 0x99);
		gint16 mode = (myfont.data[2] & 0x99);
		if ( mode & 0b001 ) {
			myfont.glyphs=512;
		} else {
			myfont.glyphs=256;
		}
//		printf("FONT[0-4]: x%02x x%02x x%02x x%02x\n",myfont.data[0] & 0xff, myfont.data[1] & 0xff, myfont.data[2] & 0xff, myfont.data[3] & 0xff );
		printf("FONT: width=%i height=%i bytes_per_row=%i glyphs=%i\n", myfont.width, myfont.height, myfont.bytes_per_row, myfont.glyphs );
	} else if (
		((myfont.data[0] & 0xff) == 0x72)
		&& ((myfont.data[1] & 0xff) == 0xb5)
		&& ((myfont.data[2] & 0xff) == 0x4a)
		&& ((myfont.data[3] & 0xff) == 0x86)
	) {
		// v2
//		printf("FONT[4-7]: x%02x x%02x x%02x x%02x\n",myfont.data[4] & 0xff, myfont.data[5] & 0xff, myfont.data[6] & 0xff, myfont.data[7] & 0xff );
//		printf("FONT[8-11]: x%02x x%02x x%02x x%02x\n",myfont.data[8] & 0xff, myfont.data[9] & 0xff, myfont.data[10] & 0xff, myfont.data[11] & 0xff );
//		printf("FONT[8-11]: x%x (version)\n", myfont.header.version);
//		printf("FONT[12-15]: x%x %i (header size)\n", myfont.header.header_size, myfont.header.header_size);
//		printf("FONT[16-19]: x%x (flags)\n", myfont.header.flags);
//		printf("FONT[20-23]: x%x (glyphs)\n", myfont.header.glyphs);
//		printf("FONT[24-27]: x%x (char size)\n", myfont.header.char_size);
//		printf("FONT[28-31]: x%x %i (height)\n", myfont.header.height, myfont.header.height);
//		printf("FONT[32-35]: x%x %i (width)\n", myfont.header.width, myfont.header.width);

		// validate version and header size
		if (myfont.header.version != 0) {
			printf("ERROR: font %s is a v2 PSF which an unknown sub-version (x%x), exiting.\n",
				filename, myfont.header.version);
			exit(5);
		}

		if (myfont.header.header_size != 32) {
			printf("ERROR: font %s is a v2 PSF which an unknown header size (x%x), exiting.\n",
				filename, myfont.header.header_size);
			exit(6);
		}

		// bytes per row
		int bytes_per_row = ceil(myfont.header.width / 8);
		if (myfont.header.char_size != myfont.header.height * bytes_per_row) {
			printf("ERROR: font %s is a v2 PSF which an internal inconsistancy char_size=%i but we calculated it should be %i, exiting.\n",
				filename, myfont.header.char_size, bytes_per_row);
			exit(6);
		}
		bytes_per_row++;
		if (bytes_per_row > 2) {
			printf("ERROR: font %s is a v2 PSF with bytes_row=%i > 2, exiting.\n",
				filename, bytes_per_row);
			exit(7);
		}
		myfont.bytes_per_row = bytes_per_row;

		myfont.glyphs = myfont.header.glyphs;
		myfont.height = myfont.header.height;
		myfont.width = myfont.header.width;
		myfont.char_size = myfont.header.char_size;

		printf("%s is a v2 PSF\n\twith %i %ix%i characters, %i bytes each for %i total bytes of chardata.\n",
			filename, myfont.glyphs, myfont.height, myfont.width, myfont.char_size,
			myfont.char_size * myfont.glyphs);

		// TODO: implement!!!
		printf("ERROR: font %s is a v2 PSF which is UNIMPLEMENTED, exiting.\n",filename);
		exit(4);
	} else {
		// wtf?
		printf("FONT[0-3]: x%02x x%02x x%02x x%02x\n",myfont.data[0] & 0xff, myfont.data[1] & 0xff, myfont.data[2] & 0xff, myfont.data[3] & 0xff );
		printf("ERROR: font %s is a not a recognized PSF version, exiting.\n",filename);
		exit(4);
	}

	unsigned int font_pos = myfont.font_start;
	for (int c = 0; c < myfont.glyphs; c++) {
#if 0
		printf("c=%i <<%c>> starts at %i:\n", c, c, font_pos );
		for(int l = 0; l < myfont.height; l++) {
			unsigned char segment = myfont.data[font_pos+l] & 0xff;
			printf("\tl=%i seg %s\n", l, byte_to_binary(segment) );
		}
#endif
		myfont.charmap[c] = font_pos;
		font_pos += myfont.height;
	}

	// clean up fh
	if (compressed) {
		pclose(fh);
	} else {
		fclose(fh);
	}

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
	const gchar *font = json_object_get_string_member(jsonObj,"font");

	// get fontpath
	fontpath = json_object_get_array_member(jsonObj,"fontpath");

	// does font exist?
	for (guint index = 0; index < json_array_get_length(fontpath); index++) {
		const gchar *thispath;
		thispath = json_array_get_string_element(fontpath, index);
		gchar * check_file = g_strconcat(thispath,"/",font,NULL);
		if ( file_exists(check_file) ) {
			font = check_file;
			printf("found font=%s\n",font);
		}
	}

	// read font in
	read_font(font); // exits on its own errors

	// get timezones
	JsonArray *timezones_handle = json_object_get_array_member(jsonObj,"timezones");
	int tz_count = 0;
	for (guint index = 0; index < json_array_get_length(timezones_handle); index++) {
		timezones[index] =  g_strconcat(json_array_get_string_element(timezones_handle, index),NULL);
		//printf("adding tz %s\n",timezones[index]);
		tz_count++;
	}
	printf("displaying %i timezones\n",tz_count);

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

unsigned int get_segment (unsigned char c, int line) {
	return myfont.data[ myfont.charmap[c] + line ];
}

int big_display (int x, int y, char *string_small, short color) {
	int maxi = columns / (myfont.width + 1);

	// color for clock digits
	for (int c=0; c < 8; c++) {
		init_pair(c,COLOR_BLACK,c);
	}
	attron(COLOR_PAIR(color));

	for (int line=0; line < myfont.height; line++) {
		for (int i=0; i < strlen(string_small) && i<maxi; i++) {
			unsigned int raw_segment = get_segment(string_small[i],line);
				
			static char b[99];
			b[0] = '\0';

			int segment = 0;
			for (int z = 128; z > 0; z >>= 1) {
				if ((raw_segment & z) == z) {
					int thisy = segment + y + i * (myfont.width + 1);
					mvprintw(x,thisy," ",b);
				}
				segment++;
			}

			int thisy = y + i * (myfont.width + 1);
			mvprintw(x,thisy,"%s",b);
		}
		x++;
	}
	attroff(COLOR_PAIR(1));
}

void display_clock() {
	int centerx,centery;
	char* time_string;
	gchar *gtime_string;
	time_t now;
	size_t width;
	while (1) {
		clear();
		centerx = rows/2;
		centery = columns/2;

		now=time(NULL);
		time_string=ctime(&now);

		GDateTime * date = g_date_time_new_now_local();

		gtime_string = g_date_time_format(date, "%H:%M:%S");
		int big_width = strlen(gtime_string) * myfont.width;
		big_display(2,centery-(big_width/2),(char *)gtime_string,COLOR_RED);

		const gchar *day = g_date_time_format(date, "%Z UTC%z %a %e %b %Y");
		width = strlen(day);
		mvprintw(2+myfont.height,centery-(width/2),"%s",day);

		int top_line = 2 + myfont.height + 2;
		// loop through timezones;
		for (guint index = 0; timezones[index] != NULL; index++) {
			char * tz = timezones[index];
				
			if (strlen(tz) && (top_line + myfont.height < rows)) {
				GTimeZone * zone = g_time_zone_new(timezones[index]);
				GDateTime * zoned_datetime = g_date_time_new_now(zone);
				gchar * zoned_time_string = g_date_time_format(zoned_datetime, "%H:%M");
				//mvprintw(top_line,4,"%s %s",zoned_time_string,timezones[index]);
				big_width = strlen(zoned_time_string) * myfont.width;
				big_display(top_line,centery-(big_width/2),(char *)zoned_time_string,COLOR_BLUE);
				top_line += myfont.height;

				day = g_date_time_format(zoned_datetime, "%Z UTC%z %a %e %b %Y");
				width = strlen(day);
				mvprintw(top_line,centery-(width/2),"%s",day);
				top_line += 2;
			}
		}

		refresh();	// Print it on to the real screen
/*		if (kbhit()) {
			return;
		}
*/

		getmaxyx(stdscr, rows, columns); // get current window size
		sleep(1);
	}
}

int main() {
	printf("Welcome to Curses Clock\n");				// Print welcome message
	printf("https://github.com/chicks-net/curses-clock\n\n");	// Print source URL

	initializations();
	initscr();	// Start curses mode

	// color or death!
	if (has_colors() == FALSE) {
		endwin();
		printf("no colors, no clock!");
		exit(64);
	}
	start_color();

	getmaxyx(stdscr, rows, columns); // get current window size
	printw("Welcome to Curses Clock\n\n");				// Reprint welcome message
	printw("https://github.com/chicks-net/curses-clock\n\n");	// Print source URL
	printw("\ninitializations complete\n");
	printw("(rows=%i columns=%i)\n", rows, columns);
	refresh();	// Print it on to the real screen
	sleep(1);
	
	display_clock(); // show the clock

	endwin();	// End curses mode

	printf("exiting because the user hit a key\n");

	return 0;
}
