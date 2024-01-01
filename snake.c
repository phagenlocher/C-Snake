#include <ncurses.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>

#define clean_exit(code) \
	endwin();            \
	exit(code);
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

// Misc. constants
#define VERSION "0.58.0 (Beta)"
#define CC_END_YEAR "2024"
#define STD_FILE_NAME ".csnake"
#define FILE_LENGTH 20 // 19 characters are needed to display the max number for long long

typedef enum Direction
{
	// No direction, standing still
	HOLD,
	// Negative y direction
	UP,
	// Positive y direction
	DOWN,
	// Positive x direction
	RIGHT,
	// Negative x direction
	LEFT
} Direction;

typedef enum UserInteraction
{
	// No interaction by the user
	NONE,
	// Request to pause the game
	PAUSE,
	// Request to restart the round
	RESTART,
	// Request to quit the program
	QUIT,
	// Direction button pressed
	DIRECTION
} UserInteraction;

typedef enum UpdateResult
{
	// Player was saved by a grace frame
	GRACE,
	// The round ends in a game over
	GAME_OVER,
	// The round may continue
	CONTINUE
} UpdateResult;

typedef struct Coord
{
	int x;
	int y;
} Coord;

typedef struct LinkedCell
{
	// Coordinate for the cell
	Coord coord;
	// The previous cell in the linked list
	// `this->prev->next` should point to `this`
	struct LinkedCell *prev;
	// The next cell in the linked list
	// `this->next->prev` should point to `this`
	struct LinkedCell *next;
} LinkedCell;

typedef struct GameState
{
	// Current score
	long long points;
	// Current direction
	Direction direction;
	// Previous direction from the last update
	Direction old_direction;
	// Current speed
	int speed;
	// Current position of the snake head
	Coord pos;
	// Position of the snake head from the last update
	Coord old_pos;
	// Current "bonus" points that would be added if food was hit
	int points_counter;
	// Current length of the snake
	int length;
	// Current amount of cells the snake still needs to grow
	int growing;
	// Amount of available update cycles that
	int grace_frames;
	// Amount of update cycles needed until the food becomes a super food
	int superfood_counter;
	// Current position of the food
	Coord food_coord;
	// Head of the snake
	LinkedCell *head;
	// Last cell of the snake
	LinkedCell *last;
	// Points to all walls as a single linked list
	LinkedCell *wall;
} GameState;

typedef struct GameConfiguration
{
	// Path to the savefile
	char *save_file_path;
	// Maximum possible speed
	unsigned int max_speed;
	// Highscore either read from savefile or updated from last game round
	long long highscore;
	// Specifies whether outer walls should be open
	bool open_bounds_flag;
	// Specifies whether menus should be skipped
	bool skip_flag;
	// Specifies whether a wall pattern (specified by `wall_pattern`) should be used
	bool wall_flag;
	// Specifies whether the savefile should be ignored
	bool ignore_flag;
	// Specifies whether the savefile should be removed
	bool remove_flag;
	// Selects a predefined pattern (if `wall_flag` is `true`)
	short wall_pattern;
	// Key-code used for direction control
	int up_key, down_key, left_key, right_key;
	// Colorpair index for the color to print the snake in
	int snake_color;
} GameConfiguration;

// Global configuration (must be initialized with `init_configuration` before use)
static GameConfiguration *config;

// Logo generated on http://www.network-science.de/ascii/
// Used font: nancyj
static const char *LOGO[] = {
	" a88888b.          .d88888b                    dP               ",
	"d8'   `88          88.    ''                   88               ",
	"88                 `Y88888b. 88d888b. .d8888b. 88  .dP  .d8888b.",
	"88        88888888       `8b 88'  `88 88'  `88 88888'   88ooood8",
	"Y8.   .88          d8'   .8P 88    88 88.  .88 88  `8b. 88.  ...",
	" Y88888P'           Y88888P  dP    dP `88888P8 dP   `YP `88888P'"};

char *init_file_path(void)
{
	// Get "HOME" environment variable
	char *home_dir = getenv("HOME");
	if (home_dir == NULL)
	{
		return NULL;
	}

	// Allocate space for the home path, the filename, '/' and the zero byte
	char *file_path = malloc(strlen(home_dir) + strlen(STD_FILE_NAME) + 2);
	sprintf(file_path, "%s/%s", home_dir, STD_FILE_NAME);
	return file_path;
}

void init_configuration(void)
{
	config = malloc(sizeof(GameConfiguration));
	config->save_file_path = init_file_path();
	config->max_speed = STD_MAX_SPEED;
	config->highscore = 0;
	config->ignore_flag = false;
	config->remove_flag = false;
	config->open_bounds_flag = false;
	config->skip_flag = false;
	config->wall_flag = false;
	config->wall_pattern = 1;
	config->snake_color = 2;
	config->up_key = KEY_UP;
	config->down_key = KEY_DOWN;
	config->left_key = KEY_LEFT;
	config->right_key = KEY_RIGHT;
}

// Write a score to the score file, reading the file path from the global config
// Returns `false` on error, `true` otherwise
bool write_score_file(long long score)
{
	// If we ignore the score file we return
	// If the file path was not initialized we also return
	if (config->ignore_flag || config->save_file_path == NULL)
	{
		return true;
	}

	// Memory for the string representation of the score
	char score_str[FILE_LENGTH];

	// Write highscore into memory
	sprintf(score_str, "%.*lld", FILE_LENGTH - 1, score);

	// Open file
	FILE *file = fopen(config->save_file_path, "w");
	if (file == NULL)
	{
		return false;
	}

	// Write score to file and close it
	fputs(score_str, file);
	fclose(file);
	return true;
}

// Read the highscore from the score file, reading the file path from the global config
// Returns `false` on error, `true` otherwise
bool read_score_file(void)
{
	// If we ignore the score file we return
	if (config->ignore_flag)
		return true;

	// If the file path was not initialized we also return
	if (config->save_file_path == NULL)
		return true;

	// File doesn't exist
	if (access(config->save_file_path, F_OK) != 0)
	{
		config->highscore = 0;
		return true;
	}

	// Memory for file contents
	char content[FILE_LENGTH];

	// Open file
	FILE *file = fopen(config->save_file_path, "r");
	if (file == NULL)
	{
		config->highscore = 0;
		return false;
	}

	// Read file contents and interpret the score
	if (fgets(content, FILE_LENGTH, file) == NULL)
		config->highscore = 0;
	else
		config->highscore = atoll(content);

	// Close the file
	fclose(file);
	return true;
}

inline Coord coord(int x, int y)
{
	Coord coord;
	coord.x = x;
	coord.y = y;
	return coord;
}

inline Coord get_max_coords(WINDOW *win)
{
	return coord(getmaxx(win), getmaxy(win));
}

inline size_t half_len(const char string[])
{
	return strlen(string) / 2;
}

void print_centered(WINDOW *window, int y, const char string[])
{
	int max_x = getmaxx(window);
	int x = (max_x / 2) - half_len(string);
	mvwaddstr(window, y, x, string);
}

void print_offset(WINDOW *window, int x_offset, int y, const char string[])
{
	int max_x = getmaxx(window);
	int x = (max_x / 2) - half_len(string);
	mvwaddstr(window, y, x + x_offset, string);
}

void print_status(WINDOW *status_win, GameState *state)
{
	char txt_buf[50];
	int max_x = getmaxx(status_win);

	// Set normal
	wattrset(status_win, A_BOLD);

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
		sprintf(txt_buf, "Score: %lld", state->points);
		mvwaddstr(status_win, 1, (max_x / 3) - half_len(txt_buf), txt_buf);

		// Print highscore
		if (config->highscore != 0)
		{
			sprintf(txt_buf, "Highscore: %lld", config->highscore);
		}
		else
		{
			sprintf(txt_buf, "No highscore set");
		}
		mvwaddstr(status_win, 1, (2 * max_x / 3) - half_len(txt_buf), txt_buf);

		// Print points counter
		sprintf(txt_buf, "Bonus: %d", state->points_counter);
		mvwaddstr(status_win, 2, (max_x / 3) - half_len(txt_buf), txt_buf);

		// Print length
		sprintf(txt_buf, "Length: %d", state->length);
		mvwaddstr(status_win, 2, (2 * max_x / 3) - half_len(txt_buf), txt_buf);
	}
	else
	{
		// Print score
		sprintf(txt_buf, "Score: %lld", state->points);
		mvwaddstr(status_win, 1, (max_x / 2) - half_len(txt_buf), txt_buf);

		// Print points counter
		sprintf(txt_buf, "Bonus: %d", state->points_counter);
		mvwaddstr(status_win, 2, (max_x / 2) - half_len(txt_buf), txt_buf);
	}

	// Refresh window
	wrefresh(status_win);
}

void pause_game(WINDOW *status_win, const char string[], const int seconds)
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
	}
	else
	{
		sleep(seconds);
		flushinp(); // Flush typeahead during sleep
	}
	// Clear window
	wclear(status_win);

	// Redrawing the box
	wattrset(status_win, A_BOLD);
	box(status_win, 0, 0);

	// Refreshing the window
	wrefresh(status_win);
}

bool is_on_obstacle(LinkedCell *test_cell, const int x, const int y)
{
	// Not a valid cell to test
	if (test_cell == NULL)
		return false;

	do
	{
		// A cell has the same coordinates as the point to check
		if ((test_cell->coord.x == x) && (test_cell->coord.y == y))
			return true;

		// Check next cell
		test_cell = test_cell->prev;
	} while (test_cell != NULL);

	// No cell found
	return false;
}

void new_random_coordinates(
	LinkedCell *snake,
	LinkedCell *wall,
	Coord *coord,
	Coord max_coords)
{
	int x, y;
	do
	{
		// Generate random coordinates
		x = rand() % max_coords.x;
		y = rand() % max_coords.y;

		// Check if the coordinates are on the snake or wall
		// If so, generate new values
	} while (is_on_obstacle(snake, x, y) || is_on_obstacle(wall, x, y));

	// Save coordinates
	coord->x = x;
	coord->y = y;
}

LinkedCell *create_wall(int start, int end, int constant, Direction dir, LinkedCell *last_cell)
{
	int i;
	LinkedCell *new_wall, *wall = malloc(sizeof(LinkedCell));

	// Based on the direction set either x or y to a constant value
	if ((dir == LEFT) || (dir == RIGHT))
	{
		wall->coord = coord(start, constant);
	}
	else
	{
		wall->coord = coord(constant, start);
	}

	// Connect the new wall to an old one
	// If last_cell is NULL the list ends here
	wall->prev = last_cell;

	// Based on the direction build a wall from start to end
	// and put it infrnt of the old list
	switch (dir)
	{
	case UP:
		for (i = start - 1; i > end; i--)
		{
			new_wall = malloc(sizeof(LinkedCell));
			new_wall->coord = coord(constant, i);
			new_wall->prev = wall;
			wall = new_wall;
		}
		break;
	case DOWN:
		for (i = start + 1; i < end; i++)
		{
			new_wall = malloc(sizeof(LinkedCell));
			new_wall->coord = coord(constant, i);
			new_wall->prev = wall;
			wall = new_wall;
		}
		break;
	case LEFT:
		for (i = start - 1; i > end; i--)
		{
			new_wall = malloc(sizeof(LinkedCell));
			new_wall->coord = coord(i, constant);
			new_wall->prev = wall;
			wall = new_wall;
		}
		break;
	case RIGHT:
		for (i = start + 1; i < end; i++)
		{
			new_wall = malloc(sizeof(LinkedCell));
			new_wall->coord = coord(i, constant);
			new_wall->prev = wall;
			wall = new_wall;
		}
		break;
	case HOLD:
		break;
	}

	return wall;
}

void free_linked_list(LinkedCell *cell)
{
	// Nothing valid to free
	if (cell == NULL)
		return;

	// Free list
	LinkedCell *tmp_cell;
	do
	{
		tmp_cell = cell->prev;
		free(cell);
		cell = tmp_cell;
	} while (cell != NULL);
}

int snake_char_from_direction(Direction direction, Direction old_direction)
{
	if (direction == UP)
	{
		if (old_direction == LEFT)
		{
			return ACS_LLCORNER;
		}
		else if (old_direction == RIGHT)
		{
			return ACS_LRCORNER;
		}
		else
		{
			return ACS_VLINE;
		}
	}
	else if (direction == DOWN)
	{
		if (old_direction == LEFT)
		{
			return ACS_ULCORNER;
		}
		else if (old_direction == RIGHT)
		{
			return ACS_URCORNER;
		}
		else
		{
			return ACS_VLINE;
		}
	}
	else if (direction == LEFT)
	{
		if (old_direction == UP)
		{
			return ACS_URCORNER;
		}
		else if (old_direction == DOWN)
		{
			return ACS_LRCORNER;
		}
		else
		{
			return ACS_HLINE;
		}
	}
	else if (direction == RIGHT)
	{
		if (old_direction == UP)
		{
			return ACS_ULCORNER;
		}
		else if (old_direction == DOWN)
		{
			return ACS_LLCORNER;
		}
		else
		{
			return ACS_HLINE;
		}
	}
	return 0;
}

bool update_position(GameState *state, Coord max_coord)
{
	switch (state->direction)
	{
	case UP:
		state->pos.y--;
		break;
	case DOWN:
		state->pos.y++;
		break;
	case LEFT:
		state->pos.x--;
		break;
	case RIGHT:
		state->pos.x++;
		break;
	case HOLD:
		return false;
	default:
		break;
	}

	if (config->open_bounds_flag)
	{
		// If you hit the outer bounds you'll end up on the other side
		if (state->pos.y < 0)
		{
			state->pos.y = max_coord.y - 1;
		}
		else if (state->pos.y >= max_coord.y)
		{
			state->pos.y = 0;
		}
		else if (state->pos.x < 0)
		{
			state->pos.x = max_coord.x - 1;
		}
		else if (state->pos.x >= max_coord.x)
		{
			state->pos.x = 0;
		}
		return false;
	}
	else
	{
		return (state->pos.y < 0) || (state->pos.x < 0) || (state->pos.y >= max_coord.y) || (state->pos.x >= max_coord.x);
	}
}

UserInteraction handle_input(GameState *state)
{
	// Set timeout
	timeout(state->speed);

	// Getting input
	int key = getch();

	// Save old direction
	state->old_direction = state->direction;

	// Changing direction according to the input
	if (key == config->left_key)
	{
		if (state->direction != RIGHT)
			state->direction = LEFT;
		return DIRECTION;
	}
	else if (key == config->right_key)
	{
		if (state->direction != LEFT)
			state->direction = RIGHT;
		return DIRECTION;
	}
	else if (key == config->up_key)
	{
		if (state->direction != DOWN)
			state->direction = UP;
		return DIRECTION;
	}
	else if (key == config->down_key)
	{
		if (state->direction != UP)
			state->direction = DOWN;
		return DIRECTION;
	}
	else if (key == '\n') // Enter-key
	{
		return PAUSE;
	}
	else if (key == 'R')
	{
		return RESTART;
	}
	else if (key == 'Q')
	{
		return QUIT;
	}
	return NONE;
}

void paint_objects(WINDOW *game_win, GameState *state)
{
	// Paint food
	wattrset(game_win, COLOR_PAIR((state->superfood_counter == 0) ? 4 : 3) | A_BOLD);
	mvwaddch(game_win, state->food_coord.y, state->food_coord.x, '0');
	// Paint the snake in the specified color
	wattrset(game_win, COLOR_PAIR(config->snake_color) | A_BOLD);
	int snake_char = snake_char_from_direction(state->direction, state->old_direction);
	if (snake_char)
	{
		mvwaddch(game_win, state->old_pos.y, state->old_pos.x, snake_char);
	}
	// Draw head
	mvwaddch(game_win, state->pos.y, state->pos.x, 'X');
}

UpdateResult update_state(WINDOW *game_win, WINDOW *status_win, GameState *state)
{
	// Init max coordinates in relation to game window
	Coord max_coord = get_max_coords(game_win);

	// Save old coordinates
	state->old_pos = state->pos;

	// Update position and check if outer bounds were hit
	bool wall_hit = update_position(state, max_coord);

	// The snake hits something
	if (wall_hit ||
		is_on_obstacle(state->head, state->pos.x, state->pos.y) ||
		is_on_obstacle(state->wall, state->pos.x, state->pos.y))
	{
		if (state->grace_frames == 0)
		{
			// No grace frames left, game over
			return GAME_OVER;
		}
		else
		{
			// We still have grace frames so we reset the coordinate and
			// let the player change the direction
			state->grace_frames--;
			state->pos = state->old_pos;
			return GRACE;
		}
	}

	// Reset grace frames
	state->grace_frames = GRACE_FRAMES;

	// Add new head to snake
	LinkedCell *new_cell = malloc(sizeof(LinkedCell));
	new_cell->coord = state->pos;
	new_cell->prev = state->head;
	state->head->next = new_cell;
	state->head = new_cell;

	// Head hits the food
	if ((state->pos.x == state->food_coord.x) &&
		(state->pos.y == state->food_coord.y))
	{
		// Let the snake grow and change the speed
		state->growing +=
			state->superfood_counter == 0 ? SUPERFOOD_GROW_FACTOR : GROW_FACTOR;
		if (state->speed > config->max_speed)
		{
			state->speed -= SPEED_FACTOR;
		}
		state->points +=
			(state->points_counter +
			 state->length + (STARTING_SPEED - state->speed) * 5) *
			(state->superfood_counter == 0 ? 5 : 1);
		state->points_counter = POINTS_COUNTER_VALUE;
		state->superfood_counter =
			(state->superfood_counter == 0) ? SUPERFOOD_COUNTER_VALUE : state->superfood_counter - 1;
		new_random_coordinates(state->head, state->wall, &state->food_coord, max_coord);
	}

	// If the snake is not growing...
	if (state->growing == 0)
	{
		// Clear last cell
		wattrset(game_win, A_NORMAL);
		mvwaddch(game_win, state->last->coord.y, state->last->coord.x, ' ');
		// ...free the memory for this cell
		LinkedCell *new_last;
		state->last->next->prev = NULL;
		new_last = state->last->next;
		free(state->last);
		state->last = new_last;
	}
	else
	{
		// If the snake is growing and moving, just decrement 'growing'...
		state->growing--;
		// ...and increment 'length'
		state->length++;
	}

	// Decrement the points that will be added
	if (state->points_counter > MIN_POINTS)
	{
		state->points_counter--;
	}

	return CONTINUE;
}

LinkedCell *init_wall(Coord max_coord)
{
	int max_x = max_coord.x;
	int max_y = max_coord.y;
	// Creating walls (all walls are referenced by one pointer)
	LinkedCell *wall = NULL;
	if (config->wall_flag)
	{
		switch (config->wall_pattern)
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
			fprintf(stderr, "Illegal wall pattern: %d\n", config->wall_pattern);
			abort();
		}
	}
	return wall;
}

GameState init_state(Coord max_coord)
{
	// Init gamestate
	GameState state;
	state.points = 0;
	state.speed = STARTING_SPEED;
	state.pos.x = max_coord.x / 2;
	state.pos.y = max_coord.y / 2;
	state.old_pos = state.pos;
	state.points_counter = POINTS_COUNTER_VALUE;
	state.length = 1;
	state.growing = STARTING_LENGTH - 1;
	state.grace_frames = GRACE_FRAMES;
	state.direction = HOLD;
	state.old_direction = HOLD;
	state.superfood_counter = SUPERFOOD_COUNTER_VALUE;
	state.food_coord.x = 0;
	state.food_coord.y = 0;

	// Create first cell for the snake
	LinkedCell *head = malloc(sizeof(LinkedCell));
	head->coord = state.pos;
	head->prev = NULL;
	head->next = NULL;
	state.head = head;
	state.last = head;

	// Init wall
	state.wall = init_wall(max_coord);

	return state;
}

// Plays one round of the game. Can be interrupted by the user.
// Returns `true` if a reset was requested, thus another round
// should start without showing the menu.
bool play_round(void)
{
	// Init max coordinates
	int global_max_x = getmaxx(stdscr);
	int global_max_y = getmaxy(stdscr);

	// Clear screen
	clear();
	refresh();

	// Create subwindows
	WINDOW *game_win = subwin(stdscr, global_max_y - 4, global_max_x, 0, 0);
	WINDOW *status_win = subwin(stdscr, 4, global_max_x, global_max_y - 4, 0);

	// Init max coordinates in relation to game window
	Coord max_coord = get_max_coords(game_win);

	// Init gamestate
	GameState state = init_state(max_coord);
	bool did_loose = false;
	bool should_repeat = false;

	// Set initial timeout
	timeout(state.speed); // The timeout for getch() makes up the game speed

	// Print status window since points have been set to 0
	print_status(status_win, &state);

	if (state.wall != NULL)
	{
		// Paint the wall
		// It should never be overwritten, since it will not be redrawn
		LinkedCell *tmp_cell1, *tmp_cell2;
		tmp_cell2 = state.wall;
		wattrset(game_win, COLOR_PAIR(5) | A_BOLD);
		do
		{
			tmp_cell1 = tmp_cell2;
			mvwaddch(game_win, tmp_cell1->coord.y, tmp_cell1->coord.x, ACS_CKBOARD);
			tmp_cell2 = tmp_cell1->prev;
		} while (tmp_cell2 != NULL);
	}

	// Init food coordinates
	new_random_coordinates(state.head, state.wall, &state.food_coord, max_coord);

	// Game-Loop
	while (true)
	{
		// Paint snake head and food
		paint_objects(game_win, &state);

		// Update status window
		print_status(status_win, &state);

		// Refresh game window
		wrefresh(game_win);

		// Get input
		UserInteraction interact = handle_input(&state);
		if (interact == PAUSE)
		{
			wattrset(status_win, COLOR_PAIR(4) | A_BOLD);
			pause_game(status_win, "--- PAUSED ---", 0);
		}
		else if (interact == RESTART)
		{
			should_repeat = true;
			break;
		}
		else if (interact == QUIT)
		{
			clean_exit(0);
		}

		// No movement, so no update
		if (state.direction == HOLD)
		{
			continue;
		}

		// Update game state
		UpdateResult res = update_state(game_win, status_win, &state);
		if (res == GAME_OVER)
		{
			did_loose = true;
			break;
		}
	}

	// Set a new highscore
	if (state.points > config->highscore)
	{
		// Remember the highscore
		config->highscore = state.points;

		// Write highscore to local file
		if (write_score_file(state.points))
		{
			wattrset(status_win, COLOR_PAIR(2) | A_BOLD);
			pause_game(status_win, "--- NEW HIGHSCORE ---", 2);
		}
		else
		{
			endwin();
			fprintf(stderr, "Unable to write savefile at %s\n", config->save_file_path);
			fprintf(stderr, "Final highscore was: %lld\n", state.points);
			exit(1);
		}
	}

	if (did_loose)
	{
		wattrset(status_win, COLOR_PAIR(3) | A_BOLD);
		pause_game(status_win, "--- GAME OVER ---", 2);
	}

	// Freeing memory used for the snake
	free_linked_list(state.head);

	// Freeing memory used for the walls
	free_linked_list(state.wall);

	// Delete windows
	delwin(game_win);
	delwin(status_win);

	// Delete the screen content
	clear();
	refresh();

	return should_repeat;
}

void play_game(void)
{
	while (true)
	{
		bool should_repeat = play_round();
		if (!should_repeat)
			return;
	}
}

void show_options(WINDOW *options_win)
{
	int i, new_pattern, index = 0;
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
	print_offset(options_win, 10, 0, config->open_bounds_flag ? "Open" : "Closed");
	print_offset(options_win, 10, 1, config->wall_flag ? "Enabled" : "Disabled");
	sprintf(txt_buf, "%d", config->wall_pattern);
	print_offset(options_win, 10, 2, txt_buf);

	// Refresh window
	wrefresh(options_win);

	// Wait for input
	int key = getch();
	if (key == config->up_key)
	{
		if (index == 0)
			index = 3;
		else
			index--;
	}
	else if (key == config->down_key)
	{
		index = (index + 1) % 4;
	}
	else if (key == '\n')
	{
		switch (index)
		{
		case 0:
			config->open_bounds_flag = !config->open_bounds_flag;
			break;
		case 1:
			config->wall_flag = !config->wall_flag;
			break;
		case 2:
			new_pattern = (config->wall_pattern + 1) % 6;
			if (new_pattern == 0)
			{
				new_pattern = 1;
			}
			config->wall_pattern = new_pattern;
			break;
		case 3:
			return;
		}
	}

	goto option_show;
}

GameConfiguration show_startscreen(void)
{
	// Getting screen dimensions
	int max_x = getmaxx(stdscr);
	int max_y = getmaxy(stdscr);

	int i, index = 0;
	char txt_buf[40];
show:
	// Set getch to blocking mode
	timeout(-1);

	// Clear the whole screen
	clear();

	// Print logo
	attrset(COLOR_PAIR(config->snake_color) | A_BOLD);
	for (i = 0; i < 6; i++)
	{
		print_centered(stdscr, (max_y / 4) + i, LOGO[i]);
	}

	// Create subwindow under the logo with 10 lines and maximum width
	WINDOW *options_win = subwin(stdscr, 10, max_x, (max_y / 4) + i + 1, 0);

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
	if (key == config->up_key)
	{
		if (index == 0)
			index = 3;
		else
			index--;
	}
	else if (key == config->down_key)
	{
		index = (index + 1) % 4;
	}
	else if (key == '\n')
	{
		switch (index)
		{
		case 0:
			play_game();
			break;
		case 1:
			show_options(options_win);
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
			clean_exit(0);
		}
	}

	// Delete options window
	delwin(options_win);

	// Go back to the beginning
	goto show;
}

void parse_arguments(int argc, char **argv)
{
	int arg, int_arg;
	int vim_flag = false;
	int option_index = 0;
	char *string_arg;

	const struct option long_opts[] =
		{
			{"help", no_argument, NULL, 'h'},
			{"version", no_argument, NULL, 'v'},
			{"remove-savefile", no_argument, NULL, 'r'},
			{"ignore-savefile", no_argument, NULL, 'i'},
			{"open-bound", no_argument, NULL, 'o'},
			{"skip-title", no_argument, NULL, 's'},
			{"vim", no_argument, &vim_flag, true},
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
				config->max_speed = 150 - int_arg;
				break;
			}
			goto help_text;
		case 'o':
			config->open_bounds_flag = true;
			break;
		case 's':
			config->skip_flag = true;
			break;
		case 'i':
			config->ignore_flag = true;
			break;
		case 'f':
			if (config->save_file_path != NULL)
			{
				free(config->save_file_path);
			}
			string_arg = calloc(strlen(optarg) + 1, sizeof(char));
			strcpy(string_arg, optarg);
			config->save_file_path = string_arg;
			break;
		case 'r':
			config->remove_flag = true;
			break;
		case 'w':
			int_arg = atoi(optarg);
			if (in_range(int_arg, 1, 5))
			{
				config->wall_flag = true;
				config->wall_pattern = int_arg;
				break;
			}
			goto help_text;
		case 'c':
			int_arg = atoi(optarg);
			if (in_range(int_arg, 1, 5))
			{
				config->snake_color = int_arg;
				break;
			}	  // else pass to help information
		case '?': // Invalid parameter
				  // pass to help information
		case 'h':
		help_text:
			printf("Usage: %s [options]\n", argv[0]);
			printf("Options:\n");
			printf(" --open-bounds, -o\n\tOuter bounds will let the snake pass through\n");
			printf(" --walls <1-5>, -w <1-5>\n\tEnable a predefined wall pattern\n");
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
			printf("C-Snake %s\nCopyright (c) 2015-%s Philipp Hagenlocher\nLicense: MIT\nCheck source for full license text.\nThere is no warranty.\n", VERSION, CC_END_YEAR);
			exit(0);
		}
	}

	// Set up keys
	if (vim_flag)
	{
		config->up_key = 'k';
		config->down_key = 'j';
		config->left_key = 'h';
		config->right_key = 'l';
	}
}

int main(int argc, char **argv)
{
	// Globally initialize configuration
	init_configuration();

	// Parse arguments
	parse_arguments(argc, argv);

	// Seed RNG with current time
	srand(time(NULL));

	if (config->save_file_path != NULL)
	{
		// If the remove flag has been set we remove the file and exit
		if (config->remove_flag)
		{
			remove(config->save_file_path);
			exit(0);
		}
		else
		{
			// Read local highscore
			if (!read_score_file())
			{
				fprintf(stderr, "Unable to read savefile at %s\n", config->save_file_path);
				fprintf(stderr, "If the error persists try using the --ignore-savefile flag!\n");
				exit(1);
			}
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
	curs_set(false);
	noecho();
	cbreak();
	keypad(stdscr, true);

	// Get screen dimensions
	int max_x = getmaxx(stdscr);
	int max_y = getmaxy(stdscr);

	// If the screen width is smaller than 64, the logo cannot be displayed
	// and the titlescreen will most likely not work, so it is skipped.
	// If the height is smaller than 19, the version cannot be displayed
	// correctly so we have to skip the title.
	if ((max_x < 64) || (max_y < 19))
	{
		config->skip_flag = true;
	}

	// Endless loop until the user quits the game
	while (true)
	{
		if (config->skip_flag)
		{
			play_game();
		}
		else
		{
			show_startscreen();
		}
	}
}
