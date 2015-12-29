#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>

// Macro to get the x-coordinate for a string to be centered on screen
#define centered(string) ((MAX_X / 2) - (strlen(string) / 2))
// Macro to print a centered string on a window an a specific y-coordinate
#define print_centered(window, y, string) ((mvwaddstr(window, y, centered(string), string)))
// Constants important for gamplay
#define STARTING_SPEED 150
#define STARTING_LENGTH 5
#define POINTS_COUNTER_VALUE 1000
#define MIN_POINTS 100
#define MIN_SPEED 50
#define GROW_FACTOR 10
#define SPEED_FACTOR 1
#define VERSION "a0.22"
#define FILE_NAME ".csnake"
#define FILE_LENGTH 20 	// 19 characters are needed to display the max number for long long

typedef enum Direction Direction;
typedef struct LinkedCell LinkedCell;

enum Direction {HOLD, UP, DOWN, RIGHT, LEFT};

struct LinkedCell {
	int x;
	int y;
	LinkedCell *last;
	LinkedCell *next;
};

// Globals
static char *TXT_BUF; // Buffer used for format strings
static char *FILE_PATH;
static unsigned int MAX_X;
static unsigned int MAX_Y;
static unsigned int SPEED;
static long long POINTS;
static long long HIGHSCORE;
static WINDOW *GAME_WIN;
static WINDOW *STATUS_WIN;
static int OPEN_BOUNDS = FALSE;
static int SKIP_TITLE = FALSE;
static int IGNORE_FILES = FALSE;
static int SNAKE_COLOR = 2;
// Logo generated on http://www.network-science.de/ascii/
// Used font: nancyj
static const char *LOGO[] = {
	" a88888b.          .d88888b                    dP               ",
	"d8'   `88          88.    ''                   88               ", 
	"88                 `Y88888b. 88d888b. .d8888b. 88  .dP  .d8888b.",
	"88        88888888       `8b 88'  `88 88'  `88 88888'   88ooood8",
	"Y8.   .88          d8'   .8P 88    88 88.  .88 88  `8b. 88.  ...",
	" Y88888P'           Y88888P  dP    dP `88888P8 dP   `YP `88888P'"
};

void write_score_file() {
	if(IGNORE_FILES) {
		return;
	}
	char score_str[FILE_LENGTH];
	sprintf(score_str, "%lld", HIGHSCORE);
	FILE *save_file = fopen(FILE_PATH, "w");
	fputs(score_str, save_file);
	fclose(save_file);
}

void read_score_file() {
	if(IGNORE_FILES) {
		return;
	}
	char content[FILE_LENGTH];
	// Allocate space for the home path, the filename, 1 '/' and the zero byte.
	FILE_PATH = malloc(sizeof(char) * (strlen(getenv("HOME")) + strlen(FILE_NAME) + 2));
	strcat(FILE_PATH, getenv("HOME"));
	strcat(FILE_PATH, "/");
	strcat(FILE_PATH, FILE_NAME);
	FILE *save_file = fopen(FILE_PATH, "r");
	if(save_file == NULL) {
		HIGHSCORE = 0;
		return;
	}
	fgets(content, FILE_LENGTH, save_file);
	HIGHSCORE = atoll(content);
	fclose(save_file);
}

void clean_exit() {
	// Write highscore to local file
	write_score_file();
	// Free buffers
	free(TXT_BUF);
	free(FILE_PATH);
	// Delete windows
	delwin(GAME_WIN);
	delwin(STATUS_WIN);
	endwin();
	exit(0);
}

void print_points() {
	wattrset(STATUS_WIN, A_UNDERLINE  | A_BOLD);
	sprintf(TXT_BUF, "Score: %lld", POINTS);
	print_centered(STATUS_WIN, 1, TXT_BUF);
	wrefresh(STATUS_WIN);
}

void pause_game(const char string[], const int seconds) {
	print_centered(STATUS_WIN, 2, string);
	wrefresh(STATUS_WIN);

	// Pausing for 0 seconds means waiting for input
	if(seconds == 0) {
		timeout(-1); // getch is in blocking mode
		getch();
		timeout(SPEED); // getch is in nonblocking mode
	} else {
		sleep(seconds);
	}
	// Deleting second row
	wmove(STATUS_WIN, 2, 0);
	wclrtoeol(STATUS_WIN);
	// Redrawing the box
	wattrset(STATUS_WIN, A_NORMAL);
	box(STATUS_WIN, 0, 0);
	// Refreshing the window
	wrefresh(STATUS_WIN);
}

int is_on_obstacle(LinkedCell *test_cell, const int x, const int y){
	do{
		if((test_cell->x == x) && (test_cell->y == y)) {
			return TRUE;
		}
		test_cell = test_cell->last;
	} while(test_cell != NULL);
	return FALSE;
}

void new_random_coordinates(LinkedCell *test_cell, int *x, int *y) {
	do {
		*x = rand() % MAX_X;
		*y = rand() % MAX_Y;
	} while(is_on_obstacle(test_cell, *x, *y));
}

void play_round() {
	round_start:
  	// Init windows and max coordinates
	getmaxyx(stdscr, MAX_Y, MAX_X);
	GAME_WIN = subwin(stdscr, MAX_Y - 4, MAX_X, 0, 0);
	STATUS_WIN = subwin(stdscr, 4, MAX_X, MAX_Y - 4, 0);
	wattrset(STATUS_WIN, A_NORMAL);
	box(STATUS_WIN, 0, 0);
	getmaxyx(GAME_WIN, MAX_Y, MAX_X);

	// Set game specific timeout
	SPEED = STARTING_SPEED;
	timeout(SPEED); // The timeout for getch() makes up the game speed

	// Init variables
	int key, x, y, growing, food_x, food_y, points_counter, lost, repeat, length;
	Direction direction, old_direction;
	x = MAX_X / 2;
	y = MAX_Y / 2;
	POINTS = key = 0;
	points_counter = POINTS_COUNTER_VALUE;
	growing = STARTING_LENGTH - 1;
	lost = TRUE;
	repeat = FALSE;
	length = 1;
	direction = old_direction = HOLD;

	// Print points after they have been set to 0
	print_points();

	// Create first cell for the snake
	LinkedCell *last_cell, *head = malloc(sizeof(LinkedCell));
	head->x = x;
	head->y = y;
	head->last = NULL;
	head->next = NULL;
	last_cell = head;

	// Init food coordinates
	new_random_coordinates(head, &food_x, &food_y);

	// Game-Loop
	while(TRUE) {

		// Painting the food and the snakes head
		wattrset(GAME_WIN, COLOR_PAIR(3) | A_BOLD);
		mvwaddch(GAME_WIN, food_y, food_x, '0');
		wattrset(GAME_WIN, COLOR_PAIR(SNAKE_COLOR) | A_BOLD);
		mvwaddch(GAME_WIN,y,x,'X');

		// Getting input
		get_input: key = getch();

		// Changing direction according to the input
		if(key == KEY_LEFT) {
			if(direction != RIGHT)
				direction = LEFT;
		}else if(key == KEY_RIGHT) {
			if(direction != LEFT)
				direction = RIGHT;
		}else if(key == KEY_UP) {
			if(direction != DOWN)
				direction = UP;
		}else if(key == KEY_DOWN) {
			if(direction != UP)
				direction = DOWN;
		}else if((key == '\n') && (direction != HOLD)) { // Enter-key
			wattrset(STATUS_WIN, COLOR_PAIR(4) | A_BOLD);
			pause_game("--- PAUSED ---", 0);
		}else if(key == 'R') {
			lost = FALSE;
			repeat = TRUE;
			break;
		}else if(key == 'Q') {
			// If the title screen is disabled we will exit the programm
			if(SKIP_TITLE) {
				clean_exit();
			}
			lost = FALSE;
			break;
		}else if(direction == HOLD) {
			// If the snake is not moving, there is no update to be made
			goto get_input;
		}

		// Change x and y according to the direction and paint the fitting
		// character on the coordinate BEFORE changing the coordinate
		// Paint the snake in the specified color
		wattrset(GAME_WIN, COLOR_PAIR(SNAKE_COLOR) | A_BOLD);
		if(direction == UP) {
			if(old_direction == LEFT) {
				mvwaddch(GAME_WIN, y,x,ACS_LLCORNER);
			} else if(old_direction == RIGHT) {
				mvwaddch(GAME_WIN, y,x,ACS_LRCORNER);
			} else {
				mvwaddch(GAME_WIN, y,x,ACS_VLINE);
			}
			y--;
		}else if(direction == DOWN) {
			if(old_direction == LEFT) {
				mvwaddch(GAME_WIN, y,x,ACS_ULCORNER);
			} else if(old_direction == RIGHT) {
				mvwaddch(GAME_WIN, y,x,ACS_URCORNER);
			} else {
				mvwaddch(GAME_WIN, y,x,ACS_VLINE);
			}
			y++;
		}else if(direction == LEFT) {
			if(old_direction == UP) {
				mvwaddch(GAME_WIN, y,x,ACS_URCORNER);
			} else if(old_direction == DOWN) {
				mvwaddch(GAME_WIN, y,x,ACS_LRCORNER);
			} else {
				mvwaddch(GAME_WIN, y,x,ACS_HLINE);
			}
			x--;
		}else if(direction == RIGHT) {
			if(old_direction == UP) {
				mvwaddch(GAME_WIN, y,x,ACS_ULCORNER);
			} else if(old_direction == DOWN) {
				mvwaddch(GAME_WIN, y,x,ACS_LLCORNER);
			} else {
				mvwaddch(GAME_WIN, y,x,ACS_HLINE);
			}
			x++;
		}

		if(OPEN_BOUNDS) {
			// If you hit the outer bounds you'll end up on the other side
			if(y < 0) {
				y = MAX_Y;
			}else if(y >= MAX_Y) {
				y = 0;
			}else if(x < 0) {
				x = MAX_X;
			}else if(x >= MAX_X){
				x = 0;
			}
		} else {
			// If the bounds aren't open you will loose
			if((y < 0) || (x < 0)) {
				break;
			}
			if((y >= MAX_Y) || (x >= MAX_X)) {
				break;
			}
		}

		// Draw head and create new cell for the head
		mvwaddch(GAME_WIN,y,x,'X');
		LinkedCell *new_cell = malloc(sizeof(LinkedCell));
		new_cell->x = x;
		new_cell->y = y;
		new_cell->last = head;
		head->next = new_cell;
		head = new_cell;
		// If the snake is moving (game has started) and the head hit the body
		// of the snake the game is over.
		if(is_on_obstacle(head->last, x, y)) {
			break;
		}

		// Head hits the food
		if((x == food_x) && (y == food_y)) {
			// Let the snake grow and change the speed
			growing += GROW_FACTOR;
			if(SPEED > MIN_SPEED) {
				SPEED -= SPEED_FACTOR;
			}
			POINTS += points_counter + length + (STARTING_SPEED - SPEED);
			points_counter = POINTS_COUNTER_VALUE;
			timeout(SPEED);
			print_points();
			new_random_coordinates(head, &food_x, &food_y);
		}

		// If the snake is not growing...
		if(growing == 0) {
			LinkedCell *new_last;
			// ...delete the last character from the terminal...
			wattrset(GAME_WIN, A_NORMAL);
			mvwaddch(GAME_WIN, last_cell->y,last_cell->x,' ');
			// ...and free the memory for this cell.
			last_cell->next->last = NULL;
			new_last = last_cell->next;
			free(last_cell);
			last_cell = new_last;
		} else {
			// If the snake is growing and moving, just decrement 'growing'
			growing--;
		}

		// The old direction is the direction we had this round.
		old_direction = direction;

		// Decrement the points that will be added
		if(points_counter > MIN_POINTS) {
			points_counter--;
		}

		wrefresh(GAME_WIN);
	}

	// lost is set to FALSE if the player quit the game
	if(lost) {
		wattrset(STATUS_WIN, COLOR_PAIR(3) | A_BOLD);
		pause_game("--- YOU LOST ---", 2);
	}

	// Set a new highscore
	if(POINTS > HIGHSCORE) {
		HIGHSCORE = POINTS;
		// Write highscore to local file
		write_score_file();
		// Display status
		wattrset(STATUS_WIN, COLOR_PAIR(2) | A_BOLD);
		pause_game("--- NEW HIGHSCORE ---", 2);
	}

	// Freeing memory used for the snake
	LinkedCell *tmp_cell;
	do {
		tmp_cell = head->last;
		free(head);
		head = tmp_cell;
	} while(head != NULL);

	// Delete the screen content
	clear();

	// If we are repeating we are jumping to the start of the function
	if(repeat) {
		goto round_start;
	}
}

void show_startscreen() {
	int i, key, anchor;
	// Getting screen dimensions
	getmaxyx(stdscr, MAX_Y, MAX_X);
	// Setting anchor, where the text will start
	anchor = MAX_Y / 4;
	// Clear the whole screen
	clear();
	// Printing logo and instructions
	attrset(COLOR_PAIR(SNAKE_COLOR) | A_BOLD);
	for(i = 0; i<6; i++) {
		print_centered(stdscr, anchor + i, LOGO[i]);
	}
	attrset(COLOR_PAIR(1) | A_BOLD);
	print_centered(stdscr, anchor + 7, "--- (P)lay Game --- (Q)uit --- (C)redits ---");

	// If points != 0 print them to the screen
	if(POINTS != 0) {
		sprintf(TXT_BUF, "--- Last Score: %lld ---", POINTS);
		print_centered(stdscr, anchor + 8, TXT_BUF);
	}
	if(HIGHSCORE != 0) {
		sprintf(TXT_BUF, "--- Highscore: %lld ---", HIGHSCORE);
		print_centered(stdscr, anchor + 9, TXT_BUF);
	}
	// Displaying information on open bounds
	if(OPEN_BOUNDS) {
		print_centered(stdscr, anchor + 10, "--- Playing with open bounds! ---");
	}
	if(IGNORE_FILES) {
		print_centered(stdscr, anchor + 11, "--- Savefile is ignored! ---");
	}

	// Printing verion
	sprintf(TXT_BUF, "Version: %s", VERSION);
	print_centered(stdscr, MAX_Y - 1, TXT_BUF);

	// Wait for input
	timeout(-1); // Set getch to blocking mode
	while(TRUE) {
		key = getch();
		if((key == 'P') || (key == 'p')) {
			clear();
			play_round();
			break;
		}else if((key == 'Q') || (key == 'q')) {
			clean_exit();
		}else if((key == 'C') || (key == 'c')) {
			for(i = 0; i<5; i++) {
				move(anchor + 7 + i, 0);
				clrtoeol();
			}
			print_centered(stdscr, anchor + 7, "--- Programming by Philipp Hagenlocher ---");
			print_centered(stdscr, anchor + 8, "--- Start with -v to get information on the license ---");
			print_centered(stdscr, anchor + 9, "--- Press any key! ---");
			refresh();
			getch();
			break;
		}
	}
}

void parse_arguments(int argc, char **argv) {
	int arg, color;
	while((arg = getopt(argc, argv, "osihvc:")) != -1) {
		switch (arg) {
			case 'o':
				OPEN_BOUNDS = TRUE;
				break;
			case 's':
				SKIP_TITLE = TRUE;
				break;
			case 'i':
				IGNORE_FILES = TRUE;
				break;
			case 'c':
				color = atoi(optarg);
				if((color >= 1) && (color <= 5)) {
					SNAKE_COLOR = color;
					break;
				}
			case '?': // Invalid parameter
				// pass to help information
			case 'h':
				printf("Usage: %s [options]\n", argv[0]);
				printf("Options:\n");
				printf(" -o\tOuter bounds will let the snake pass through\n");
				printf(" -c <1-5>\n\tSet the snakes color:\n\t1 = White\n\t2 = Green\n\t3 = Red\n\t4 = Yellow\n\t5 = Blue\n");
				printf(" -s\tSkip the titlescreen\n");
				printf(" -i\tIgnore savefile (don't read or write)\n");
				printf(" -h\tDisplay this information\n");
				printf(" -v\tDisplay version and license information\n");
				exit(0);
			case 'v':
				printf("C-Snake %s\nCopyright (c) 2015 Philipp Hagenlocher\nLicense: MIT\nCheck source for full license text or visit https://opensource.org/licenses/MIT\nThere is no warranty.\n", VERSION);
				exit(0);
		}
	}
}

int main(int argc, char **argv) {
	// Parse arguments
	parse_arguments(argc, argv);
	// Seed RNG with current time
	srand(time(NULL));
	// Read local highscore
	read_score_file();
	// Init colors and ncurses specific functions
	initscr();
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);
	init_pair(4, COLOR_YELLOW, COLOR_BLACK);
	init_pair(5, COLOR_BLUE, COLOR_BLACK);
  	bkgd(COLOR_PAIR(1));
  	curs_set(FALSE);
	noecho();
	cbreak();
	keypad(stdscr, TRUE);

	// Getting screen dimensions
	getmaxyx(stdscr, MAX_Y, MAX_X);
	// If the screen width is smaller than 64, the logo cannot be displayed
	// and the titlescreen will most likely not work, so it is skipped.
	// If the height is smaller than 17, the version cannot be displayed
	// correctly so we have to skip the title.
	if((MAX_X < 64) || (MAX_Y < 17)) {
		SKIP_TITLE = TRUE;
	}
	// Allocating memory for buffer so that the text can be big enough to fill
	// a whole row. Since it is always used to display centered text it doesn't
	// have to be bigger.
	TXT_BUF = malloc(sizeof(char) * MAX_X);

	// Endless loop until the user quits the game
	while(TRUE) {
		if(SKIP_TITLE) {
			play_round();
		} else {
  			show_startscreen();
  		}
  	}

}
