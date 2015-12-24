#include <ncurses.h>
#include <stdlib.h>
#include <time.h>

#define UP 1
#define DOWN 2
#define RIGHT 3
#define LEFT 4

typedef struct SnakeCell SnakeCell;

struct SnakeCell {
	int x;
	int y;
	SnakeCell *last;
	SnakeCell *next;
};

int main(void) {

	initscr();
	int MAX_X, MAX_Y, SPEED = 150;
	getmaxyx(stdscr, MAX_Y, MAX_X);

	// Init ncurses
	timeout(SPEED);
	curs_set(0);
	noecho();
	cbreak();
	keypad(stdscr, TRUE);

	// Init variables
	srand(time(NULL));
	int direction, old_direction, key, x, y, growing, food_x, food_y;
	x = MAX_X / 2;
	y = MAX_Y / 2;
	key = direction = old_direction = 0;
	growing = 4;
	do {
		food_x = rand() % MAX_X;
		food_y = rand() % MAX_Y;
	} while((food_x == x) && (food_y == y));
	SnakeCell *cell = malloc(sizeof(SnakeCell));
	cell->x = x;
	cell->y = y;
	cell->last = NULL;
	cell->next = NULL;

	// Game-Loop
	while(TRUE) {

		mvaddch(food_y, food_x, '0');

		key = getch();
		//addch(' ');
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

		if(y < 0) {
			y = MAX_Y;
		}else if(y >= MAX_Y) {
			y = 0;
		}else if(x < 0) {
			x = MAX_X;
		}else if(x >= MAX_X){
			x = 0;
		}


		mvaddch(y,x,'X');
		SnakeCell *new_cell = malloc(sizeof(SnakeCell));
		new_cell->x = x;
		new_cell->y = y;
		new_cell->last = cell;
		cell->next = new_cell;
		cell = new_cell;
		if((direction >= UP) && (direction <= LEFT)) {
			if(onSnake(cell->last, x, y)) {
				return endwin();
			}
		}

		if((x == food_x) & (y == food_y)) {
			growing = 20;
			SPEED -= 5;
			if (SPEED < 50) {
				SPEED = 50;
			}
			timeout(SPEED);
			do {
				food_x = rand() % MAX_X;
				food_y = rand() % MAX_Y;
			} while(onSnake(cell, food_x, food_y));
		}

		if(growing == 0) {
			SnakeCell *last_cell = cell->last;
			while(last_cell->last != NULL) {
				last_cell = last_cell->last;
			}
			mvaddch(last_cell->y,last_cell->x,' ');
			last_cell->next->last = NULL;
			free(last_cell);
		} else {
			if((direction >= UP) && (direction <= LEFT))
				growing--;
		}
		old_direction = direction;
		refresh();
	}

	return endwin();

}

int onSnake(SnakeCell *test_cell, int x, int y){
	do{
		if((test_cell->x == x) && (test_cell->y == y)) {
			return TRUE;
		}
		test_cell = test_cell->last;
	} while(test_cell != NULL);
	return FALSE;
}
