#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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
// Version as string
#define VERSION "a0.11"

typedef enum Direction Direction;
typedef struct SnakeCell SnakeCell;

enum Direction {HOLD, UP, DOWN, RIGHT, LEFT};

struct SnakeCell {
	int x;
	int y;
	SnakeCell *last;
	SnakeCell *next;
};

// Globals
static char *TXT_BUF; // Buffer used for format strings
static unsigned int MAX_X;
static unsigned int MAX_Y;
static unsigned int SPEED;
static unsigned long POINTS;
static unsigned long HIGHSCORE;
static WINDOW *GAME_WIN;
static WINDOW *STATUS_WIN;
static int OPEN_BOUNDS = FALSE;
static int SNAKE_COLOR = 2;
static const char *LOGO[] = {
	" a88888b.          .d88888b                    dP               ",
	"d8'   `88          88.    ''                   88               ", 
	"88                 `Y88888b. 88d888b. .d8888b. 88  .dP  .d8888b.",
	"88        88888888       `8b 88'  `88 88'  `88 88888'   88ooood8",
	"Y8.   .88          d8'   .8P 88    88 88.  .88 88  `8b. 88.  ...",
	" Y88888P'           Y88888P  dP    dP `88888P8 dP   `YP `88888P'"
};

void clean_exit() {
	free(TXT_BUF);
	delwin(GAME_WIN);
	delwin(STATUS_WIN);
	endwin();
	exit(0);
}

void print_points() {
	wattrset(STATUS_WIN, A_UNDERLINE  | A_BOLD);
	sprintf(TXT_BUF, "Score: %lu", POINTS);
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

int is_on_snake(SnakeCell *test_cell, const int x, const int y){
	do{
		if((test_cell->x == x) && (test_cell->y == y)) {
			return TRUE;
		}
		test_cell = test_cell->last;
	} while(test_cell != NULL);
	return FALSE;
}

void new_random_coordinates(SnakeCell *test_cell, int *x, int *y) {
	do {
		*x = rand() % MAX_X;
		*y = rand() % MAX_Y;
	} while(is_on_snake(test_cell, *x, *y));
}

void play_round() {
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
	int key, x, y, growing, food_x, food_y, points_counter, lost, length;
	Direction direction, old_direction;
	x = MAX_X / 2;
	y = MAX_Y / 2;
	POINTS = key = 0;
	points_counter = POINTS_COUNTER_VALUE;
	growing = STARTING_LENGTH - 1;
	lost = TRUE;
	length = 1;
	direction = old_direction = HOLD;

	// Print points after they have been set to 0
	print_points();

	// Create first cell for the snake
	SnakeCell *cell = malloc(sizeof(SnakeCell));
	cell->x = x;
	cell->y = y;
	cell->last = NULL;
	cell->next = NULL;

	// Init food coordinates
	new_random_coordinates(cell, &food_x, &food_y);

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
		}else if(key == '\n') { // Enter-key
			wattrset(STATUS_WIN, COLOR_PAIR(4) | A_BOLD);
			pause_game("--- PAUSED ---", 0);
		}else if(key == 'Q') {
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
		SnakeCell *new_cell = malloc(sizeof(SnakeCell));
		new_cell->x = x;
		new_cell->y = y;
		new_cell->last = cell;
		cell->next = new_cell;
		cell = new_cell;
		// If the snake is moving (game has started) and the head hit the body
		// of the snake the game is over.
		if(is_on_snake(cell->last, x, y)) {
			break;
		}

		// Head hits the food
		if((x == food_x) && (y == food_y)) {
			// Let the snake grow and change the speed
			growing += GROW_FACTOR;
			if(SPEED > MIN_SPEED) {
				SPEED -= SPEED_FACTOR;
			}
			POINTS += points_counter + length;
			points_counter = POINTS_COUNTER_VALUE;
			timeout(SPEED);
			print_points();
			new_random_coordinates(cell, &food_x, &food_y);
		}

		// If the snake is not growing...
		if(growing == 0) {
			// ...get the last cell...
			SnakeCell *last_cell = cell->last;
			while(last_cell->last != NULL) {
				last_cell = last_cell->last;
			}
			// ...delete the character from the terminal...
			wattrset(GAME_WIN, A_NORMAL);
			mvwaddch(GAME_WIN, last_cell->y,last_cell->x,' ');
			// ...and free the memory for this cell.
			last_cell->next->last = NULL;
			free(last_cell);
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
	}

	// Freeing memory used for the snake
	SnakeCell *tmp_cell;
	while(cell->last != NULL) {
		tmp_cell = cell->last;
		free(cell);
		cell = tmp_cell;
	}
	free(cell);

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
		sprintf(TXT_BUF, "--- Last Score: %lu ---", POINTS);
		print_centered(stdscr, anchor + 8, TXT_BUF);
	}
	if(HIGHSCORE != 0) {
		sprintf(TXT_BUF, "--- Highscore: %lu ---", HIGHSCORE);
		print_centered(stdscr, anchor + 9, TXT_BUF);
	}
	// Displaying information on open bounds
	if(OPEN_BOUNDS) {
		print_centered(stdscr, anchor + 10, "--- Playing with open bounds! ---");
	}

	// Printing verion
	sprintf(TXT_BUF, "Version: %s", VERSION);
	print_centered(stdscr, MAX_Y - 1, TXT_BUF);

	// Wait for input
	timeout(-1);
	while(TRUE) {
		key = getch();
		if((key == 'P') || (key == 'p')) {
			clear();
			play_round();
			break;
		}else if((key == 'Q') || (key == 'q')) {
			clean_exit();
		}else if((key == 'C') || (key == 'c')) {
			for(i = 0; i<4; i++) {
				move(anchor + 7 + i, 0);
				clrtoeol();
			}
			print_centered(stdscr, anchor + 7, "--- Programmed by Philipp Hagenlocher ---");
			print_centered(stdscr, anchor + 8, "--- Start with -v to get information on the license ---");
			print_centered(stdscr, anchor + 9, "--- Press any key! ---");
			refresh();
			timeout(-1);
			getch();
			break;
		}
	}
}

void parse_arguments(int argc, char **argv) {
	int arg, color;
	while((arg = getopt(argc, argv, "ohvc:")) != -1) {
		switch (arg) {
			case 'o':
				OPEN_BOUNDS = TRUE;
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
	// Allocating memory for buffer so that the text can be big enough to fill
	// a whole row. Since it is always used to display centered text it doesn't
	// have to be bigger.
	TXT_BUF = malloc(sizeof(char) * MAX_X);
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

	// Endless loop until the user quits the game
	while(TRUE) {
  		show_startscreen();
  	}

}
