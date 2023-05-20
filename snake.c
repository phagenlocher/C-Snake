#include <ncurses.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>

// Direction macros
#define is_horizontal(direction) ((direction == LEFT) || (direction == RIGHT))
// Misc. macros
#define in_range(x, min, max) (x >= min) && (x <= max)

// Constants important for gameplay
#define STARTING_SPEED 150
#define STARTING_LENGTH 5
#define POINTS_COUNTER_VALUE 1000
#define SUPERFOOD_COUNTER_VALUE 10
#define MIN_POINTS 100
#define STD_MAX_SPEED 50
#define GROW_FACTOR 10
#define SUPERFOOD_GROW_FACTOR 15
#define SPEED_FACTOR 2
#define GRACE_FRAMES 3
#define VERSION "0.57.1 (Beta)"
#define STD_FILE_NAME ".csnake"
#define FILE_LENGTH 20 // 19 characters are needed to display the max number for long long

enum Direction
{
	HOLD,
	UP,
	DOWN,
	RIGHT,
	LEFT
};

typedef struct linked_cell_t
{
	int x;
	int y;
	struct linked_cell_t *last;
	struct linked_cell_t *next;
} linked_cell_t;

// Globals
static char *file_path = NULL;
static unsigned int max_x;
static unsigned int max_y;
static unsigned int speed;
static unsigned int max_speed = STD_MAX_SPEED;
static long long points = 0;
static long long highscore = 0;
static WINDOW *game_win;
static WINDOW *status_win;
static WINDOW *options_win;
static int remove_flag = FALSE;
static int ignore_flag = FALSE;
static int custom_flag = FALSE;
static int open_bounds_flag = FALSE;
static int skip_flag = FALSE;
static int wall_flag = FALSE;
static int wall_pattern = 0;
static int snake_color = 2;
static int up_key, down_key, left_key, right_key;
// Logo generated on http://www.network-science.de/ascii/
// Used font: nancyj
static const char *LOGO[] = {
	" a88888b.          .d88888b                    dP               ",
	"d8'   `88          88.    ''                   88               ",
	"88                 `Y88888b. 88d888b. .d8888b. 88  .dP  .d8888b.",
	"88        88888888       `8b 88'  `88 88'  `88 88888'   88ooood8",
	"Y8.   .88          d8'   .8P 88    88 88.  .88 88  `8b. 88.  ...",
	" Y88888P'           Y88888P  dP    dP `88888P8 dP   `YP `88888P'"};

int init_file_path(void)
{
	// The path has been set somehow, so we don't overwrite it
	// This is an error
	if (file_path != NULL)
		return -1;

	// Get "HOME" environment variable
	char *home_dir = getenv("HOME");

	// We couldn't get the environment variable so we ignore the savefile
	if (home_dir == NULL)
	{
		ignore_flag = TRUE;
		file_path = NULL;
		return -1;
	}

	// Allocate space for the home path, the filename, '/' and the zero byte
	file_path = malloc(strlen(home_dir) + strlen(STD_FILE_NAME) + 2);

	// Write path to the global variable
	if (sprintf(file_path, "%s/%s", home_dir, STD_FILE_NAME) < 1)
	{
		// If something went wrong, deallocate memory and ignore savefile
		free(file_path);
		ignore_flag = TRUE;
		file_path = NULL;
		return -1;
	}

	return 1;
}

void write_score_file(void)
{
	// If we ignore the score file we return
	if (ignore_flag)
		return;

	// Memory for the string representation of the score
	char score_str[FILE_LENGTH];

	// Write highscore into memory
	sprintf(score_str, "%.*lld", FILE_LENGTH - 1, highscore);

	// Open file
	FILE *file = fopen(file_path, "w");

	// Something went wrong
	// TODO: Have error handling
	if (file == NULL)
		return;

	// Write score to file and close it
	fputs(score_str, file);
	fclose(file);
}

void read_score_file(void)
{
	// If we ignore the score file we return
	if (ignore_flag)
		return;

	// Memory for file contents
	char content[FILE_LENGTH];

	// Open file
	FILE *file = fopen(file_path, "r");

	// Something went wrong
	// TODO: Have error handling
	if (file == NULL)
	{
		highscore = 0;
		return;
	}

	// Read file contents and interpret the score
	if (fgets(content, FILE_LENGTH, file) == NULL)
		highscore = 0;
	else
		highscore = atoll(content);

	// Close the file
	fclose(file);
}

void clean_exit(void)
{
	// Write highscore to file
	write_score_file();

	// End ncurses
	endwin();

	// The OS handles the rest
	exit(0);
}

inline size_t half_len(const char string[])
{
	return strlen(string) / 2;
}

void print_centered(WINDOW *window, int y, const char string[])
{
	int x = (max_x / 2) - half_len(string);
	mvwaddstr(window, y, x, string);
}

void print_offset(WINDOW *window, int x_offset, int y, const char string[])
{
	int x = (max_x / 2) - half_len(string);
	mvwaddstr(window, y, x + x_offset, string);
}

void print_status(int length, int points_counter)
{
	char txt_buf[50];

	// Deleting rows
	wmove(status_win, 1, 0);
	wclrtoeol(status_win);
	wmove(status_win, 2, 0);
	wclrtoeol(status_win);

	// Redrawing the box
	box(status_win, 0, 0);

	// Set bold font
	wattrset(status_win, A_BOLD);

	if (max_x > 50)
	{
		// Print score
		sprintf(txt_buf, "Score: %lld", points);
		mvwaddstr(status_win, 1, (max_x / 3) - half_len(txt_buf), txt_buf);

		// Print highscore
		sprintf(txt_buf, "Highscore: %lld", highscore);
		mvwaddstr(status_win, 1, (2 * max_x / 3) - half_len(txt_buf), txt_buf);

		// Print points counter
		sprintf(txt_buf, "Bonus: %d", points_counter);
		mvwaddstr(status_win, 2, (max_x / 3) - half_len(txt_buf), txt_buf);

		// Print length
		sprintf(txt_buf, "Length: %d", length);
		mvwaddstr(status_win, 2, (2 * max_x / 3) - half_len(txt_buf), txt_buf);
	}
	else
	{
		// Print score
		sprintf(txt_buf, "Score: %lld", points);
		mvwaddstr(status_win, 1, (max_x / 2) - half_len(txt_buf), txt_buf);

		// Print points counter
		sprintf(txt_buf, "Bonus: %d", points_counter);
		mvwaddstr(status_win, 2, (max_x / 2) - half_len(txt_buf), txt_buf);
	}

	// Refresh window
	wrefresh(status_win);
}

void pause_game(const char string[], const int seconds)
{
	// Clear status window
	wclear(status_win);

	// Redrawing the box
	box(status_win, 0, 0);

	// Print the message
	print_centered(status_win, 1, string);

	// Refresh the window
	wrefresh(status_win);

	// Pausing for 0 seconds means waiting for input
	if (seconds == 0)
	{
		timeout(-1); // getch is in blocking mode
		getch();
		timeout(speed); // getch is in nonblocking mode
	}
	else
	{
		sleep(seconds);
		flushinp(); // Flush typeahead during sleep
	}
	// Clear window
	wclear(status_win);

	// Redrawing the box
	wattrset(status_win, A_NORMAL);
	box(status_win, 0, 0);

	// Refreshing the window
	wrefresh(status_win);
}

int is_on_obstacle(linked_cell_t *test_cell, const int x, const int y)
{
	// Not a valid cell to test
	if (test_cell == NULL)
		return FALSE;

	do
	{
		// A cell has the same coordinates as the point to check
		if ((test_cell->x == x) && (test_cell->y == y))
			return TRUE;

		// Check next cell
		test_cell = test_cell->last;
	} while (test_cell != NULL);

	// No cell found
	return FALSE;
}

void new_random_coordinates(linked_cell_t *snake, linked_cell_t *wall, int *x, int *y)
{
	do
	{
		// Generate random coordinates
		*x = rand() % max_x;
		*y = rand() % max_y;

		// Check if the coordinates are on the snake or wall
		// If so, generate new values
	} while (is_on_obstacle(snake, *x, *y) || is_on_obstacle(wall, *x, *y));
}

linked_cell_t *create_wall(int start, int end, int constant, enum Direction dir, linked_cell_t *last_cell)
{
	int i;
	linked_cell_t *new_wall, *wall = malloc(sizeof(linked_cell_t));

	// Based on the direction set either x or y to a constant value
	if (is_horizontal(dir))
	{
		wall->x = start;
		wall->y = constant;
	}
	else
	{
		wall->x = constant;
		wall->y = start;
	}

	// Connect the new wall to an old one
	// If last_cell is NULL the list ends here
	wall->last = last_cell;

	// Based on the direction build a wall from start to end
	// and put it infrnt of the old list
	switch (dir)
	{
	case UP:
		for (i = start - 1; i > end; i--)
		{
			new_wall = malloc(sizeof(linked_cell_t));
			new_wall->x = constant;
			new_wall->y = i;
			new_wall->last = wall;
			wall = new_wall;
		}
		break;
	case DOWN:
		for (i = start + 1; i < end; i++)
		{
			new_wall = malloc(sizeof(linked_cell_t));
			new_wall->x = constant;
			new_wall->y = i;
			new_wall->last = wall;
			wall = new_wall;
		}
		break;
	case LEFT:
		for (i = start - 1; i > end; i--)
		{
			new_wall = malloc(sizeof(linked_cell_t));
			new_wall->x = i;
			new_wall->y = constant;
			new_wall->last = wall;
			wall = new_wall;
		}
		break;
	case RIGHT:
		for (i = start + 1; i < end; i++)
		{
			new_wall = malloc(sizeof(linked_cell_t));
			new_wall->x = i;
			new_wall->y = constant;
			new_wall->last = wall;
			wall = new_wall;
		}
		break;
	case HOLD:
		break;
	}

	// Paint the wall
	// It should never be overwritten, since it will not be redrawn
	linked_cell_t *tmp_cell1, *tmp_cell2;
	tmp_cell2 = wall;
	wattrset(game_win, COLOR_PAIR(5) | A_BOLD);
	do
	{
		tmp_cell1 = tmp_cell2;
		mvwaddch(game_win, tmp_cell1->y, tmp_cell1->x, ACS_CKBOARD);
		tmp_cell2 = tmp_cell1->last;
	} while (tmp_cell2 != NULL);

	return wall;
}

void free_linked_list(linked_cell_t *cell)
{
	// Nothing valid to free
	if (cell == NULL)
		return;

	// Free list
	linked_cell_t *tmp_cell;
	do
	{
		tmp_cell = cell->last;
		free(cell);
		cell = tmp_cell;
	} while (cell != NULL);
}

void play_round(void)
{
round_start:
	// Init max coordinates
	getmaxyx(stdscr, max_y, max_x);

	// Create subwindows and their coordinates
	game_win = subwin(stdscr, max_y - 4, max_x, 0, 0);
	status_win = subwin(stdscr, 4, max_x, max_y - 4, 0);
	wattrset(status_win, A_NORMAL);
	box(status_win, 0, 0);

	// Init max coordinates in relation to game window
	getmaxyx(game_win, max_y, max_x);

	// Set globals
	points = 0;
	speed = STARTING_SPEED;
	timeout(speed); // The timeout for getch() makes up the game speed

	// Init key
	int key = 0;

	// Init gameplay variables
	int x, y, old_x, old_y;
	int points_counter, lost, repeat, length, growing, grace_frames, wall_hit;
	x = old_x = max_x / 2;
	y = old_y = max_y / 2;
	points_counter = POINTS_COUNTER_VALUE;
	lost = TRUE;
	repeat = FALSE;
	length = 1;
	growing = STARTING_LENGTH - 1;
	grace_frames = GRACE_FRAMES;
	wall_hit = FALSE;

	// Init food variables
	int superfood_counter, food_x, food_y;
	superfood_counter = SUPERFOOD_COUNTER_VALUE;
	food_x = food_y = 0;

	// Init direction variables
	enum Direction direction, old_direction;
	direction = old_direction = HOLD;

	// Print status window since points have been set to 0
	print_status(length, points_counter);

	// Create first cell for the snake
	linked_cell_t *last_cell, *head = malloc(sizeof(linked_cell_t));
	head->x = x;
	head->y = y;
	head->last = NULL;
	head->next = NULL;
	last_cell = head;

	// Creating walls (all walls are referenced by one pointer)
	linked_cell_t *wall = NULL;
	int bigger, smaller, constant;
	if (wall_flag)
	{
		switch (wall_pattern)
		{
		case 1:
			wall = create_wall(0, max_y / 4, max_x / 2, DOWN, NULL);
			wall = create_wall(max_y, 3 * max_y / 4, max_x / 2, UP, wall);
			wall = create_wall(0, max_x / 4, max_y / 2, RIGHT, wall);
			wall = create_wall(max_x, 3 * max_x / 4, max_y / 2, LEFT, wall);
			break;
		case 2:
			wall = create_wall(max_y / 4, 3 * max_y / 4, max_x / 4, DOWN, NULL);
			wall = create_wall(max_y / 4, 3 * max_y / 4, 3 * max_x / 4, DOWN, wall);
			break;
		case 3:
			wall = create_wall(max_x / 4, 3 * max_x / 4, max_y / 4, RIGHT, NULL);
			wall = create_wall(max_x / 4, 3 * max_x / 4, 3 * max_y / 4, RIGHT, wall);
			break;
		case 4:
			wall = create_wall(max_y / 2 + 2, 3 * max_y / 4, max_x / 4, DOWN, NULL);
			wall = create_wall(max_y / 4, max_y / 2 - 1, max_x / 4, DOWN, wall);
			wall = create_wall(max_y / 2 + 2, 3 * max_y / 4, 3 * max_x / 4, DOWN, wall);
			wall = create_wall(max_y / 4, max_y / 2 - 1, 3 * max_x / 4, DOWN, wall);
			wall = create_wall(max_x / 4, max_x / 2 - 1, max_y / 4, RIGHT, wall);
			wall = create_wall(max_x / 4, max_x / 2 - 1, 3 * max_y / 4, RIGHT, wall);
			wall = create_wall(max_x / 2 + 2, 3 * max_x / 4, max_y / 4, RIGHT, wall);
			wall = create_wall(max_x / 2 + 2, 3 * max_x / 4 + 1, 3 * max_y / 4, RIGHT, wall);
			break;
		case 5:
			wall = create_wall(0, max_y / 4, max_x / 4, DOWN, NULL);
			wall = create_wall(0, max_y / 4, 3 * max_x / 4, DOWN, wall);
			wall = create_wall(0, max_y / 4, max_x / 2, DOWN, wall);
			wall = create_wall(max_y, 3 * max_y / 4, max_x / 4, UP, wall);
			wall = create_wall(max_y, 3 * max_y / 4, 3 * max_x / 4, UP, wall);
			wall = create_wall(max_y, 3 * max_y / 4, max_x / 2, UP, wall);
			wall = create_wall(0, max_x / 4, max_y / 2, RIGHT, wall);
			wall = create_wall(max_x, 3 * max_x / 4, max_y / 2, LEFT, wall);
			break;
		default:
			// Random wall creation
			constant = rand() % (max_y / 2 - 1);
			do
			{
				// Since (smaller - 1) is used for modulo division
				// it has to be bigger than 1
				smaller = (rand() % (max_x - 4)) / 2;
			} while (smaller <= 1);
			bigger = max_x - smaller;
			wall = create_wall(smaller, bigger, constant, RIGHT, NULL);
			wall = create_wall(smaller, bigger, max_y - constant - 1, RIGHT, wall);

			constant = rand() % (smaller - 1);
			smaller = (rand() % max_y) / 2;
			bigger = max_y - smaller + 1;
			wall = create_wall(smaller, bigger, constant, DOWN, wall);
			wall = create_wall(smaller, bigger, max_x - constant, DOWN, wall);
			break;
		}
	}

	// Init food coordinates
	new_random_coordinates(head, wall, &food_x, &food_y);

	// Game-Loop
	while (TRUE)
	{

		// Painting the food and the snakes head
		wattrset(game_win, COLOR_PAIR((superfood_counter == 0) ? 4 : 3) | A_BOLD);
		mvwaddch(game_win, food_y, food_x, '0');
		wattrset(game_win, COLOR_PAIR(snake_color) | A_BOLD);
		mvwaddch(game_win, y, x, 'X');

		// Refresh game window
		wrefresh(game_win);

	get_input:
		// Getting input
		key = getch();

		// Changing direction according to the input
		if (key == left_key)
		{
			if (direction != RIGHT)
				direction = LEFT;
		}
		else if (key == right_key)
		{
			if (direction != LEFT)
				direction = RIGHT;
		}
		else if (key == up_key)
		{
			if (direction != DOWN)
				direction = UP;
		}
		else if (key == down_key)
		{
			if (direction != UP)
				direction = DOWN;
		}
		else if ((key == '\n') && (direction != HOLD))
		{ // Enter-key
			wattrset(status_win, COLOR_PAIR(4) | A_BOLD);
			pause_game("--- PAUSED ---", 0);
		}
		else if (key == 'R')
		{
			lost = FALSE;
			repeat = TRUE;
			break;
		}
		else if (key == 'Q')
		{
			// If the title screen is disabled we will exit the programm
			if (skip_flag)
			{
				clean_exit();
			}
			lost = FALSE;
			break;
		}
		else if (direction == HOLD)
		{
			// If the snake is not moving, there is no update to be made
			goto get_input;
		}

		// Save old coordinates
		old_x = x;
		old_y = y;

		// Change x and y according to the direction and paint the fitting
		// character on the coordinate BEFORE changing the coordinate
		// Paint the snake in the specified color
		wattrset(game_win, COLOR_PAIR(snake_color) | A_BOLD);
		if (direction == UP)
		{
			if (old_direction == LEFT)
			{
				mvwaddch(game_win, y, x, ACS_LLCORNER);
			}
			else if (old_direction == RIGHT)
			{
				mvwaddch(game_win, y, x, ACS_LRCORNER);
			}
			else
			{
				mvwaddch(game_win, y, x, ACS_VLINE);
			}
			y--;
		}
		else if (direction == DOWN)
		{
			if (old_direction == LEFT)
			{
				mvwaddch(game_win, y, x, ACS_ULCORNER);
			}
			else if (old_direction == RIGHT)
			{
				mvwaddch(game_win, y, x, ACS_URCORNER);
			}
			else
			{
				mvwaddch(game_win, y, x, ACS_VLINE);
			}
			y++;
		}
		else if (direction == LEFT)
		{
			if (old_direction == UP)
			{
				mvwaddch(game_win, y, x, ACS_URCORNER);
			}
			else if (old_direction == DOWN)
			{
				mvwaddch(game_win, y, x, ACS_LRCORNER);
			}
			else
			{
				mvwaddch(game_win, y, x, ACS_HLINE);
			}
			x--;
		}
		else if (direction == RIGHT)
		{
			if (old_direction == UP)
			{
				mvwaddch(game_win, y, x, ACS_ULCORNER);
			}
			else if (old_direction == DOWN)
			{
				mvwaddch(game_win, y, x, ACS_LLCORNER);
			}
			else
			{
				mvwaddch(game_win, y, x, ACS_HLINE);
			}
			x++;
		}

		wall_hit = FALSE;
		if (open_bounds_flag)
		{
			// If you hit the outer bounds you'll end up on the other side
			if (y < 0)
			{
				y = max_y - 1;
			}
			else if (y >= max_y)
			{
				y = 0;
			}
			else if (x < 0)
			{
				x = max_x - 1;
			}
			else if (x >= max_x)
			{
				x = 0;
			}
		}
		else
		{
			// If the bounds aren't open you will loose
			wall_hit = (y < 0) || (x < 0) || (y >= max_y) || (x >= max_x);
		}

		// The snake hits something
		if (wall_hit || is_on_obstacle(head, x, y) || is_on_obstacle(wall, x, y))
		{
			if (grace_frames == 0)
			{
				// No grace frames left, game over
				break;
			}
			else
			{
				// We still have grace frames so we reset the coordinate and
				// let the player change the direction
				grace_frames--;
				x = old_x;
				y = old_y;
				goto get_input;
			}
		}

		// Reset grace frames
		grace_frames = GRACE_FRAMES;

		// Add new head to snake but don't draw it since it might hit something
		linked_cell_t *new_cell = malloc(sizeof(linked_cell_t));
		new_cell->x = x;
		new_cell->y = y;
		new_cell->last = head;
		head->next = new_cell;
		head = new_cell;

		// Draw head
		mvwaddch(game_win, y, x, 'X');

		// Head hits the food
		if ((x == food_x) && (y == food_y))
		{
			// Let the snake grow and change the speed
			growing += superfood_counter == 0 ? SUPERFOOD_GROW_FACTOR : GROW_FACTOR;
			if (speed > max_speed)
			{
				speed -= SPEED_FACTOR;
			}
			points += (points_counter + length + (STARTING_SPEED - speed) * 5) * (superfood_counter == 0 ? 5 : 1);
			points_counter = POINTS_COUNTER_VALUE;
			timeout(speed);
			superfood_counter = (superfood_counter == 0) ? SUPERFOOD_COUNTER_VALUE : superfood_counter - 1;
			new_random_coordinates(head, wall, &food_x, &food_y);
		}

		// If the snake is not growing...
		if (growing == 0)
		{
			linked_cell_t *new_last;
			// ...replace the last character from the terminal with space...
			wattrset(game_win, A_NORMAL);
			mvwaddch(game_win, last_cell->y, last_cell->x, ' ');
			// ...and free the memory for this cell.
			last_cell->next->last = NULL;
			new_last = last_cell->next;
			free(last_cell);
			last_cell = new_last;
		}
		else
		{
			// If the snake is growing and moving, just decrement 'growing'...
			growing--;
			// ...and increment 'length'
			length++;
		}

		// The old direction is the direction we had this round.
		old_direction = direction;

		// Decrement the points that will be added
		if (points_counter > MIN_POINTS)
		{
			points_counter--;
		}

		// Update status window
		print_status(length, points_counter);

		// Refresh game window
		wrefresh(game_win);
	}

	// lost is set to FALSE if the player quit the game
	if (lost)
	{
		wattrset(status_win, COLOR_PAIR(3) | A_BOLD);
		pause_game("--- GAME OVER ---", 2);
	}

	// Set a new highscore
	if (points > highscore)
	{
		highscore = points;
		// Write highscore to local file
		write_score_file();
		// Display status
		wattrset(status_win, COLOR_PAIR(2) | A_BOLD);
		pause_game("--- NEW HIGHSCORE ---", 2);
	}

	// Freeing memory used for the snake
	free_linked_list(head);

	// Freeing memory used for the walls
	free_linked_list(wall);

	// Delete windows
	delwin(game_win);
	delwin(status_win);

	// Delete the screen content
	clear();
	refresh();

	// If we are repeating we are jumping to the start of the function
	if (repeat)
	{
		goto round_start;
	}
}

void show_options(void)
{
	int i, index = 0;
	char txt_buf[30];

option_show:
	// Clear options window
	wclear(options_win);

	const char *options[] = {
		"     Borders:",
		"       Walls:",
		"Wall-pattern:",
		"     Back    "};

	// Print options
	for (i = 0; i < 4; i++)
	{
		// Highlight the correct entry
		wattrset(options_win, COLOR_PAIR(i == index ? 7 : 1));
		print_offset(options_win, -10, i, options[i]);
	}

	// Print values for options
	wattrset(options_win, COLOR_PAIR(1));
	print_offset(options_win, 10, 0, open_bounds_flag ? "Open" : "Closed");
	print_offset(options_win, 10, 1, wall_flag ? "Enabled" : "Disabled");
	sprintf(txt_buf, "%d", wall_pattern);
	print_offset(options_win, 10, 2, txt_buf);

	// Refresh window
	wrefresh(options_win);

	// Wait for input
	int key = getch();
	if (key == up_key)
	{
		if (index == 0)
			index = 3;
		else
			index--;
	}
	else if (key == down_key)
	{
		index = (index + 1) % 4;
	}
	else if (key == '\n')
	{
		switch (index)
		{
		case 0:
			open_bounds_flag = !open_bounds_flag;
			break;
		case 1:
			wall_flag = !wall_flag;
			break;
		case 2:
			wall_pattern = (wall_pattern + 1) % 6;
			break;
		case 3:
			return;
		}
	}

	goto option_show;
}

void show_startscreen(void)
{
	int i, index = 0;
	char txt_buf[40];
show:
	// Getting screen dimensions
	getmaxyx(stdscr, max_y, max_x);

	// Set getch to blocking mode
	timeout(-1);

	// Clear the whole screen
	clear();

	// Print logo
	attrset(COLOR_PAIR(snake_color) | A_BOLD);
	for (i = 0; i < 6; i++)
	{
		print_centered(stdscr, (max_y / 4) + i, LOGO[i]);
	}

	// Create subwindow under the logo with 10 lines and maximum width
	options_win = subwin(stdscr, 10, max_x, (max_y / 4) + i + 1, 0);

	// Clear whatever is in the options window and refresh it
	wclear(options_win);
	wrefresh(options_win);

	const char *options[] = {
		"Play Game",
		"Options",
		"Credits",
		"Quit"};

	// Print options
	for (i = 0; i < 4; i++)
	{
		// Highlight correct entry
		wattrset(options_win, COLOR_PAIR(i == index ? 7 : 1));
		print_centered(options_win, i, options[i]);
	}

	// Print verion
	attrset(COLOR_PAIR(1) | A_BOLD);
	sprintf(txt_buf, "Version: %s", VERSION);
	print_centered(stdscr, max_y - 1, txt_buf);

	// Wait for input
	int key = getch();
	if (key == up_key)
	{
		if (index == 0)
			index = 3;
		else
			index--;
	}
	else if (key == down_key)
	{
		index = (index + 1) % 4;
	}
	else if (key == '\n')
	{
		switch (index)
		{
		case 0:
			clear();
			refresh();
			play_round();
			break;
		case 1:
			show_options();
			break;
		case 2:
			print_centered(options_win, 0, "Programming by Philipp Hagenlocher");
			print_centered(options_win, 1, "Please report any bugs you can find on GitHub");
			print_centered(options_win, 2, "Start with -v to get information on the license");
			print_centered(options_win, 3, "Press any key!");
			wrefresh(options_win);
			getch();
			break;
		case 3:
			clean_exit();
		}
	}

	// Delete options window
	delwin(options_win);

	// Go back to the beginning
	goto show;
}

void parse_arguments(int argc, char **argv)
{
	int arg, int_arg, vim_flag, option_index;
	option_index = 0;
	vim_flag = FALSE;

	const struct option long_opts[] =
		{
			{"help", no_argument, NULL, 'h'},
			{"version", no_argument, NULL, 'v'},
			{"remove-savefile", no_argument, NULL, 'r'},
			{"ignore-savefile", no_argument, NULL, 'i'},
			{"open-bound", no_argument, NULL, 'o'},
			{"skip-title", no_argument, NULL, 's'},
			{"vim", no_argument, &vim_flag, TRUE},
			{"color", required_argument, NULL, 'c'},
			{"walls", required_argument, NULL, 'w'},
			{"filepath", required_argument, NULL, 'f'},
			{"maximum-speed", required_argument, NULL, 1},
		};

	while ((arg = getopt_long(argc, argv, "osif:rw:c:hv", long_opts, &option_index)) != -1)
	{
		switch (arg)
		{
		case 1:
			int_arg = atoi(optarg);
			if (in_range(int_arg, 0, 150))
			{
				max_speed = 150 - int_arg;
				break;
			}
			goto help_text;
		case 'o':
			open_bounds_flag = TRUE;
			break;
		case 's':
			skip_flag = TRUE;
			break;
		case 'i':
			ignore_flag = TRUE;
			break;
		case 'f':
			file_path = optarg;
			custom_flag = TRUE;
			break;
		case 'r':
			remove_flag = TRUE;
			break;
		case 'w':
			int_arg = atoi(optarg);
			if (in_range(int_arg, 0, 5))
			{
				wall_flag = TRUE;
				wall_pattern = int_arg;
				break;
			}
			goto help_text;
		case 'c':
			int_arg = atoi(optarg);
			if (in_range(int_arg, 1, 5))
			{
				snake_color = int_arg;
				break;
			}	  // else pass to help information
		case '?': // Invalid parameter
				  // pass to help information
		case 'h':
		help_text:
			printf("Usage: %s [options]\n", argv[0]);
			printf("Options:\n");
			printf(" --open-bounds, -o\n\tOuter bounds will let the snake pass through\n");
			printf(" --walls <0-5>, -w <0-5>\n\tEnable a predefined wall pattern (0 is random!)\n");
			printf(" --color <1-5>, -c <1-5>\n\tSet the snakes color:\n\t1 = White\n\t2 = Green\n\t3 = Red\n\t4 = Yellow\n\t5 = Blue\n");
			printf(" --skip-title, -s\n\tSkip the title screen\n");
			printf(" --remove-savefile, -r\n\tRemove the savefile and quit\n");
			printf(" --ignore-savefile, -i\n\tIgnore savefile (don't read nor write)\n");
			printf(" --filepath path, -f path\n\tSpecify alternate path savefile\n");
			printf(" --vim\n\tUse vim-style direction controls (H,J,K,L)\n");
			printf(" --maximum-speed <0,150>\n\tSpecify a new maximum speed (default: 100)\n");
			printf(" --help, -h\n\tDisplay this information\n");
			printf(" --version, -v\n\tDisplay version and license information\n\n");
			printf("In-game Controls:\n");
			printf(" Arrow-Keys\n\tDirection to go\n");
			printf(" Enter\n\tPause\n");
			printf(" Shift+Q\n\tEnd Round\n");
			printf(" Shift+R\n\tRestart Round (can be used to resize the game after window size has changed)\n");
			exit(0);
		case 'v':
			printf("C-Snake %s\nCopyright (c) 2015-2023 Philipp Hagenlocher\nLicense: MIT\nCheck source for full license text.\nThere is no warranty.\n", VERSION);
			exit(0);
		}
	}

	// Set up keys
	up_key = vim_flag ? 'k' : KEY_UP;
	down_key = vim_flag ? 'j' : KEY_DOWN;
	left_key = vim_flag ? 'h' : KEY_LEFT;
	right_key = vim_flag ? 'l' : KEY_RIGHT;
}

int main(int argc, char **argv)
{
	// Parse arguments
	parse_arguments(argc, argv);

	// Seed RNG with current time
	srand(time(NULL));

	// Init path for score file
	if (init_file_path() == 1)
	{
		// If the remove flag has been set we remove the file and exit
		if (remove_flag)
		{
			remove(file_path);
			exit(0);
		}
		else
		{
			// Read local highscore
			read_score_file();
		}
	}

	// Init colors and ncurses specific functions
	initscr();
	start_color();
	short background = COLOR_BLACK;
	if (use_default_colors() != ERR)
	{
		background = -1;
	}
	init_pair(1, COLOR_WHITE, background);
	init_pair(2, COLOR_GREEN, background);
	init_pair(3, COLOR_RED, background);
	init_pair(4, COLOR_YELLOW, background);
	init_pair(5, COLOR_BLUE, background);
	init_pair(6, COLOR_BLACK, COLOR_GREEN);
	init_pair(7, COLOR_BLACK, COLOR_RED);
	init_pair(8, COLOR_BLACK, COLOR_YELLOW);
	init_pair(9, COLOR_BLACK, COLOR_BLUE);
	bkgd(COLOR_PAIR(1));
	curs_set(FALSE);
	noecho();
	cbreak();
	keypad(stdscr, TRUE);

	// Getting screen dimensions
	getmaxyx(stdscr, max_y, max_x);

	// If the screen width is smaller than 64, the logo cannot be displayed
	// and the titlescreen will most likely not work, so it is skipped.
	// If the height is smaller than 19, the version cannot be displayed
	// correctly so we have to skip the title.
	if ((max_x < 64) || (max_y < 19))
	{
		skip_flag = TRUE;
	}

	// Endless loop until the user quits the game
	while (TRUE)
	{
		if (skip_flag)
		{
			play_round();
		}
		else
		{
			show_startscreen();
		}
	}
}
