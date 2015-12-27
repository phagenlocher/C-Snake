#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Macro to get the x-coordinate for a string to be centered on screen
#define centered(string) ((MAX_X / 2) - (strlen(string) / 2))

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
static char VERSION[] = "a0.1";
static unsigned int MAX_X;
static unsigned int MAX_Y;
static unsigned int SPEED;
static unsigned long POINTS;
static WINDOW *GAME_WIN;
static WINDOW *STATUS_WIN;
static int OPEN_BOUNDS = FALSE;
static int SNAKE_COLOR = 2;

void clean_exit() {
	delwin(GAME_WIN);
	delwin(STATUS_WIN);
	delwin(stdscr);
	endwin();
	exit(0);
}

void print_points() {
	wattrset(STATUS_WIN, A_UNDERLINE  | A_BOLD);
	char *points_text = malloc(sizeof(char) * MAX_X);
	sprintf(points_text, "Score: %lu", POINTS);
	mvwaddstr(STATUS_WIN, 1, centered(points_text), points_text);
	free(points_text);
	wrefresh(STATUS_WIN);
}

void pause_game(const char string[], const int seconds) {
	int i;
	mvwaddstr(STATUS_WIN, 2, centered(string), string);
	wrefresh(STATUS_WIN);
	
	if(seconds == 0) {
		timeout(-1);
		getch();
		timeout(SPEED);
	} else {
		sleep(seconds);
	}
	for(i = 1; i<MAX_X-1; i++) {
		mvwaddch(STATUS_WIN, 2, i, ' ');
	}
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
	box(STATUS_WIN, 0, 0);
	getmaxyx(GAME_WIN, MAX_Y, MAX_X);

	// Set game specific timeout
	SPEED = 150;
	timeout(SPEED); // The timeout for getch() makes up the game speed

	// Init RNG with current time
	srand(time(NULL));

	// Init variables
	int key, x, y, growing, food_x, food_y, points_counter, lost;
	Direction direction, old_direction;
	x = MAX_X / 2;
	y = MAX_Y / 2;
	POINTS = key = 0;
	points_counter = 1000;
	growing = 4;
	lost = TRUE;
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
			growing += 10;
			if(SPEED > 50) {
				SPEED -= 5;
			}
			POINTS += points_counter;
			points_counter = 1000;
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
		if(points_counter > 100) {
			points_counter--;
		}

		wrefresh(GAME_WIN);
	}

	if(lost) {
		wattrset(STATUS_WIN, COLOR_PAIR(3) | A_BOLD);
		pause_game("--- YOU LOST ---", 2);
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
	int i, key;
	getmaxyx(stdscr, MAX_Y, MAX_X);
	const char *logo[] = {
	" a88888b.          .d88888b                    dP               ",
	"d8'   `88          88.    ''                   88               ", 
	"88                 `Y88888b. 88d888b. .d8888b. 88  .dP  .d8888b.",
	"88        88888888       `8b 88'  `88 88'  `88 88888'   88ooood8",
	"Y8.   .88          d8'   .8P 88    88 88.  .88 88  `8b. 88.  ...",
	" Y88888P'           Y88888P  dP    dP `88888P8 dP   `YP `88888P'"};
	const char instruction[] = "--- (P)lay Game --- (Q)uit ---";
	clear();
	// Printing logo and instructions
	attrset(COLOR_PAIR(SNAKE_COLOR) | A_BOLD);
	for(i = 0; i<6; i++) {
		mvaddstr(MAX_Y / 2 - (MAX_Y / 4) + i, centered(logo[i]), logo[i]);
	}
	attrset(COLOR_PAIR(1) | A_BOLD);
	mvaddstr(MAX_Y / 2 - (MAX_Y / 4) + 7, centered(instruction), instruction);

	// If points != 0 print them to the screen
	if(POINTS != 0) {
		char *points_text = malloc(sizeof(char) * MAX_X);
		sprintf(points_text, "--- Last Score: %lu ---", POINTS);
		mvaddstr(MAX_Y / 2 - (MAX_Y / 4) + 8, centered(points_text), points_text);
		free(points_text);
	}

	// Wait for input
	timeout(-1);
	attrset(A_NORMAL);
	while(TRUE) {
		key = getch();
		if((key == 'P') || (key == 'p')) {
			clear();
			play_round();
			break;
		}else if((key == 'Q') || (key == 'q')) {
			clean_exit();
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