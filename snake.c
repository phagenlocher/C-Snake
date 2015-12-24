#include <ncurses.h>
#include <stdlib.h>
#include <time.h>

#define UP 1
#define DOWN 2
#define RIGHT 3
#define LEFT 4
#define is_moving(direction) ((direction >= UP) && (direction <= LEFT))

typedef struct SnakeCell SnakeCell;

struct SnakeCell {
	int x;
	int y;
	SnakeCell *last;
	SnakeCell *next;
};

static int MAX_X;
static int MAX_Y;

void new_random_coordinates(SnakeCell *test_cell, int *x, int *y) {
	do {
		*x = rand() % MAX_X;
		*y = rand() % MAX_Y;
	} while(is_on_snake(test_cell, *x, *y));
}

int is_on_snake(SnakeCell *test_cell, int x, int y){
	do{
		if((test_cell->x == x) && (test_cell->y == y)) {
			return TRUE;
		}
		test_cell = test_cell->last;
	} while(test_cell != NULL);
	return FALSE;
}

int main(void) {

	initscr();
	int SPEED = 150;
	getmaxyx(stdscr, MAX_Y, MAX_X);

	// Init ncurses
	timeout(SPEED); // The timeout for getch() makes up the game speed
	curs_set(FALSE);
	noecho();
	cbreak();
	keypad(stdscr, TRUE);

	// Init RNG with current time
	srand(time(NULL));

	// Init variables
	int direction, old_direction, key, x, y, growing, food_x, food_y;
	x = MAX_X / 2;
	y = MAX_Y / 2;
	key = direction = old_direction = 0;
	growing = 4;

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
		mvaddch(food_y, food_x, '0');

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
		}

		// Change x and y according to the direction and paint the fitting
		// character on the coordinate BEFORE changing the coordinate
		if(direction == UP) {
			if(old_direction == LEFT) {
				mvaddch(y,x,ACS_LLCORNER);
			} else if(old_direction == RIGHT) {
				mvaddch(y,x,ACS_LRCORNER);
			} else {
				mvaddch(y,x,ACS_VLINE);
			}
			y--;
		}else if(direction == DOWN) {
			if(old_direction == LEFT) {
				mvaddch(y,x,ACS_ULCORNER);
			} else if(old_direction == RIGHT) {
				mvaddch(y,x,ACS_URCORNER);
			} else {
				mvaddch(y,x,ACS_VLINE);
			}
			y++;
		}else if(direction == LEFT) {
			if(old_direction == UP) {
				mvaddch(y,x,ACS_URCORNER);
			} else if(old_direction == DOWN) {
				mvaddch(y,x,ACS_LRCORNER);
			} else {
				mvaddch(y,x,ACS_HLINE);
			}
			x--;
		}else if(direction == RIGHT) {
			if(old_direction == UP) {
				mvaddch(y,x,ACS_ULCORNER);
			} else if(old_direction == DOWN) {
				mvaddch(y,x,ACS_LLCORNER);
			} else {
				mvaddch(y,x,ACS_HLINE);
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
		mvaddch(y,x,'X');
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
				return endwin();
			}
		}

		// Head hits the food
		if((x == food_x) & (y == food_y)) {
			// Let the snake grow and change the speed
			growing = 20;
			SPEED -= 5;
			if (SPEED < 50) {
				SPEED = 50;
			}
			timeout(SPEED);
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
			mvaddch(last_cell->y,last_cell->x,' ');
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
		refresh();
	}

	return endwin();

}