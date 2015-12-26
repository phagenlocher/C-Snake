#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define is_moving(direction) (direction != HOLD)
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

static int MAX_X;
static int MAX_Y;
static int SPEED;
static int POINTS;
static WINDOW *GAME_WIN;
static WINDOW *STATUS_WIN;

void clean_exit() {
	delwin(GAME_WIN);
	delwin(STATUS_WIN);
	delwin(stdscr);
	endwin();
	exit(0);
}

void start_screen() {
	int i, key;
	getmaxyx(stdscr, MAX_Y, MAX_X);
	char *logo[] = {
	" a88888b.          .d88888b                    dP               ",
	"d8'   `88          88.    ''                   88               ", 
	"88                 `Y88888b. 88d888b. .d8888b. 88  .dP  .d8888b.",
	"88        88888888       `8b 88'  `88 88'  `88 88888'   88ooood8",
	"Y8.   .88          d8'   .8P 88    88 88.  .88 88  `8b. 88.  ...",
	" Y88888P'           Y88888P  dP    dP `88888P8 dP   `YP `88888P'"};
	clear();
	attrset(COLOR_PAIR(2) | A_BOLD);
	for(i = 0; i<6; i++) {
		mvaddstr(MAX_Y / 2 - 20 + i, centered(logo[i]), logo[i]);
	}
	attrset(COLOR_PAIR(1) | A_BOLD);
	char instruction[] = "--- (P)lay Game --- (Q)uit ---";
	char text[] = "--- Last Points: TODO ---";
	mvaddstr(MAX_Y / 2 - 20 + 7, centered(instruction), instruction);
	mvaddstr(MAX_Y / 2 - 20 + 8, centered(text), text);
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

void print_points() {
	wattrset(STATUS_WIN, A_UNDERLINE  | A_BOLD);
	mvwprintw(STATUS_WIN, 1, MAX_X / 2 - 4, "Points: %d", POINTS);
	wrefresh(STATUS_WIN);
}

void pause(const char string[], const int seconds) {
	int i;
	mvwaddstr(STATUS_WIN, 2, centered(string), string);
	wrefresh(STATUS_WIN);
	// Set getch to 'blocking'-mode
	timeout(-1);
	sleep(seconds);
	getch();
	for(i = 1; i<MAX_X-1; i++) {
		mvwaddch(STATUS_WIN, 2, i, ' ');
	}
	wrefresh(STATUS_WIN);
	timeout(SPEED);
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
	print_points(0);

	// Set game specific timeout
	SPEED = 150;
	timeout(SPEED); // The timeout for getch() makes up the game speed

	// Init RNG with current time
	srand(time(NULL));

	// Init variables
	int key, x, y, growing, food_x, food_y, points_counter;
	Direction direction, old_direction;
	x = MAX_X / 2;
	y = MAX_Y / 2;
	POINTS = key = 0;
	points_counter = 200;
	growing = 4;
	direction = old_direction = HOLD;

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

		// Painting the food
		wattrset(GAME_WIN, COLOR_PAIR(3) | A_BOLD);
		mvwaddch(GAME_WIN, food_y, food_x, '0');

		// Getting input
		key = getch();

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
			pause("--- PAUSED ---", 0);
		}else if(key == 'Q') {
			break;
		}

		// Change x and y according to the direction and paint the fitting
		// character on the coordinate BEFORE changing the coordinate
		wattrset(GAME_WIN, COLOR_PAIR(2) | A_BOLD); // Paints the snake green
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
		if(is_moving(direction)) {
			if(is_on_snake(cell->last, x, y)) {
				wattrset(STATUS_WIN, COLOR_PAIR(3) | A_BOLD);
				pause("--- YOU LOST ---", 1);
				break;
			}
		}

		// Head hits the food
		if((x == food_x) && (y == food_y)) {
			// Let the snake grow and change the speed
			growing += 10;
			if(SPEED > 50) {
				SPEED -= 5;
			}
			POINTS += points_counter;
			points_counter = 200;
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
			if(is_moving(direction)) {
				growing--;
			}
		}

		// The old direction is the direction we had this round.
		old_direction = direction;

		// Decrement the points that will be added
		if(points_counter > 10) {
			points_counter--;
		}

		wrefresh(GAME_WIN);
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

int main(void) {

	// Init colors and ncurses specific functions
	initscr();
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK); 
	init_pair(2, COLOR_GREEN, COLOR_BLACK); 
	init_pair(3, COLOR_RED, COLOR_BLACK); 
	init_pair(4, COLOR_YELLOW, COLOR_BLACK); 
  	bkgd(COLOR_PAIR(1));
  	curs_set(FALSE);
	noecho();
	cbreak();
	keypad(stdscr, TRUE);

	// Endless loop until the user quits the game
	while(TRUE) {
  		start_screen();
  	}

}