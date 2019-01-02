#include <ncurses.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>

// Macro to get the x-coordinate for a string to be centered on screen
#define centered(string) ((max_x / 2) - (strlen(string) / 2))
// Macro to print a centered string on a window an a specific y-coordinate
#define print_centered(window, y, string) (mvwaddstr(window, y, centered(string), string))
// Direction macros
#define is_horizontal(direction) ((direction == LEFT) || (direction == RIGHT))
#define is_vertical(direction) ((direction == UP) || (direction == DOWN))

// Constants important for gamplay
#define STARTING_SPEED 150
#define STARTING_LENGTH 5
#define POINTS_COUNTER_VALUE 1000
#define SUPERFOOD_COUNTER_VALUE 10
#define MIN_points 100
#define MIN_SPEED 50
#define GROW_FACTOR 10
#define SPEED_FACTOR 2
#define VERSION "0.53 (Beta)"
#define STD_FILE_NAME ".csnake"
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
static char *txt_buf; // Buffer used for format strings
static char *file_path = NULL;
static unsigned int max_x;
static unsigned int max_y;
static unsigned int speed;
static long long points;
static long long highscore;
static WINDOW *game_win;
static WINDOW *status_win;
static int remove_flag = FALSE;
static int ignore_flag = FALSE;
static int custom_flag = FALSE;
static int open_bounds_flag = FALSE;
static int skip_flag = FALSE;
static int wall_flag = FALSE;
static int wall_pattern;
static int vim_flag = FALSE;
static int snake_color = 2;
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

void init_file_path() {
	if(file_path != NULL) {
		return;
	}
	// Get "HOME" environment variable
	char *home_dir = getenv("HOME");
	if(home_dir == NULL) {
		ignore_flag = TRUE;
		file_path = NULL;
		return;
	}
	// Allocate space for the home path, the filename, '/' and the zero byte
	file_path = malloc(strlen(home_dir) + strlen(STD_FILE_NAME) + 2);
	if(sprintf(file_path, "%s/%s", home_dir, STD_FILE_NAME) < 1) {
		free(file_path);
		file_path = NULL;
	}
}

void write_score_file() {
	if(ignore_flag) {
		return;
	}
	char score_str[FILE_LENGTH];
	sprintf(score_str, "%.*lld", FILE_LENGTH-1, highscore);
	FILE *file = fopen(file_path, "w");
	if(file == NULL) {
		return;
	}
	fputs(score_str, file);
	fclose(file);
}

void read_score_file() {
	if(ignore_flag) {
		return;
	}
	char content[FILE_LENGTH];
	FILE *file = fopen(file_path, "r");
	if(file == NULL) {
		highscore = 0;
		return;
	}
	if(fgets(content, FILE_LENGTH, file) == NULL) {
		highscore = 0;
	} else {
		highscore = atoll(content);
	}
	fclose(file);
}

void clean_exit() {
	// Write highscore to file
	write_score_file();
	// End ncurses
	endwin();
	// The OS handles the rest
	exit(0);
}

void print_points() {
	wattrset(status_win, A_UNDERLINE | A_BOLD);
	sprintf(txt_buf, "Score: %lld", points);
	print_centered(status_win, 1, txt_buf);
	wrefresh(status_win);
}

void pause_game(const char string[], const int seconds) {
	print_centered(status_win, 2, string);
	wrefresh(status_win);

	// Pausing for 0 seconds means waiting for input
	if(seconds == 0) {
		timeout(-1); // getch is in blocking mode
		getch();
		timeout(speed); // getch is in nonblocking mode
	} else {
		sleep(seconds);
		flushinp(); // Flush typeahead during sleep
	}
	// Deleting second row
	wmove(status_win, 2, 0);
	wclrtoeol(status_win);
	// Redrawing the box
	wattrset(status_win, A_NORMAL);
	box(status_win, 0, 0);
	// Refreshing the window
	wrefresh(status_win);
}

int is_on_obstacle(LinkedCell *test_cell, const int x, const int y){
	if(test_cell == NULL) {
		return FALSE;
	}

	do{
		if((test_cell->x == x) && (test_cell->y == y)) {
			return TRUE;
		}
		test_cell = test_cell->last;
	} while(test_cell != NULL);
	return FALSE;
}

void new_random_coordinates(LinkedCell *snake, LinkedCell *wall, int *x, int *y) {
	do {
		*x = rand() % max_x;
		*y = rand() % max_y;
	} while(is_on_obstacle(snake, *x, *y) || is_on_obstacle(wall, *x, *y));
}

LinkedCell *create_wall(int start, int end, int constant, Direction dir, LinkedCell *last_cell) {
	int i;
	LinkedCell *new_wall, *wall = malloc(sizeof(LinkedCell));

	if(is_horizontal(dir)) {
		wall->x = start;
		wall->y = constant;
	} else {
		wall->x = constant;
		wall->y = start;
	}
	wall->last = last_cell;

	switch (dir) {
		case UP:
			for(i = start-1; i > end; i--) {
				new_wall = malloc(sizeof(LinkedCell));
				new_wall->x = constant;
				new_wall->y = i;
				new_wall->last = wall;
				wall = new_wall;
			}
			break;
		case DOWN:
			for(i = start+1; i < end; i++) {
				new_wall = malloc(sizeof(LinkedCell));
				new_wall->x = constant;
				new_wall->y = i;
				new_wall->last = wall;
				wall = new_wall;
			}
			break;
		case LEFT:
			for(i = start-1; i > end; i--) {
				new_wall = malloc(sizeof(LinkedCell));
				new_wall->x = i;
				new_wall->y = constant;
				new_wall->last = wall;
				wall = new_wall;
			}
			break;
		case RIGHT:
			for(i = start+1; i < end; i++) {
				new_wall = malloc(sizeof(LinkedCell));
				new_wall->x = i;
				new_wall->y = constant;
				new_wall->last = wall;
				wall = new_wall;
			}
			break;
		case HOLD:
			break;
	}

	LinkedCell *tmp_cell1, *tmp_cell2;
	tmp_cell2 = wall;
	wattrset(game_win, COLOR_PAIR(5) | A_BOLD);
	do {
		tmp_cell1 = tmp_cell2;
		mvwaddch(game_win, tmp_cell1->y, tmp_cell1->x, ACS_CKBOARD);
		tmp_cell2 = tmp_cell1->last;
	} while(tmp_cell2 != NULL);

	return wall;
}

void free_linked_list(LinkedCell *cell) {
	if(cell == NULL) {
		return;
	}

	LinkedCell *tmp_cell;
	do {
		tmp_cell = cell->last;
		free(cell);
		cell = tmp_cell;
	} while(cell != NULL);
}

void play_round() {
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

	// Init key variables
	int key, up_key, down_key, left_key, right_key;
	key = 0;
	up_key = vim_flag ? 'k' : KEY_UP;
	down_key = vim_flag ? 'j' : KEY_DOWN;
	left_key = vim_flag ? 'h' : KEY_LEFT;
	right_key = vim_flag ? 'l' : KEY_RIGHT;

	// Init gameplay variables
	int x, y, points_counter, lost, repeat, length, growing;
	x = max_x / 2;
	y = max_y / 2;
	points_counter = POINTS_COUNTER_VALUE;
	lost = TRUE;
	repeat = FALSE;
	length = 1;
	growing = STARTING_LENGTH - 1;

	// Init food variables
	int superfood_counter, food_x, food_y;
	superfood_counter = SUPERFOOD_COUNTER_VALUE;
	food_x = food_y = 0;

	// Init direction variables
	Direction direction, old_direction;
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

	// Creating walls (all walls are referenced by one pointer)
	LinkedCell *wall = NULL;
	int bigger, smaller, constant;
	if(wall_flag) {
		switch (wall_pattern) {
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
				do {
					// Since (smaller - 1) is used for modulo division
					// it has to be bigger than 1
					smaller = (rand() % (max_x - 4)) / 2;
				} while(smaller <= 1);
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
	while(TRUE) {

		// Painting the food and the snakes head
		wattrset(game_win, COLOR_PAIR((superfood_counter == 0) ? 4 : 3) | A_BOLD);
		mvwaddch(game_win, food_y, food_x, '0');
		wattrset(game_win, COLOR_PAIR(snake_color) | A_BOLD);
		mvwaddch(game_win, y, x, 'X');

		// Getting input
		get_input: key = getch();

		// Changing direction according to the input
		if(key == left_key) {
			if(direction != RIGHT)
				direction = LEFT;
		}else if(key == right_key) {
			if(direction != LEFT)
				direction = RIGHT;
		}else if(key == up_key) {
			if(direction != DOWN)
				direction = UP;
		}else if(key == down_key) {
			if(direction != UP)
				direction = DOWN;
		}else if((key == '\n') && (direction != HOLD)) { // Enter-key
			wattrset(status_win, COLOR_PAIR(4) | A_BOLD);
			pause_game("--- PAUSED ---", 0);
		}else if(key == 'R') {
			lost = FALSE;
			repeat = TRUE;
			break;
		}else if(key == 'Q') {
			// If the title screen is disabled we will exit the programm
			if(skip_flag) {
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
		wattrset(game_win, COLOR_PAIR(snake_color) | A_BOLD);
		if(direction == UP) {
			if(old_direction == LEFT) {
				mvwaddch(game_win, y,x,ACS_LLCORNER);
			} else if(old_direction == RIGHT) {
				mvwaddch(game_win, y,x,ACS_LRCORNER);
			} else {
				mvwaddch(game_win, y,x,ACS_VLINE);
			}
			y--;
		}else if(direction == DOWN) {
			if(old_direction == LEFT) {
				mvwaddch(game_win, y,x,ACS_ULCORNER);
			} else if(old_direction == RIGHT) {
				mvwaddch(game_win, y,x,ACS_URCORNER);
			} else {
				mvwaddch(game_win, y,x,ACS_VLINE);
			}
			y++;
		}else if(direction == LEFT) {
			if(old_direction == UP) {
				mvwaddch(game_win, y,x,ACS_URCORNER);
			} else if(old_direction == DOWN) {
				mvwaddch(game_win, y,x,ACS_LRCORNER);
			} else {
				mvwaddch(game_win, y,x,ACS_HLINE);
			}
			x--;
		}else if(direction == RIGHT) {
			if(old_direction == UP) {
				mvwaddch(game_win, y,x,ACS_ULCORNER);
			} else if(old_direction == DOWN) {
				mvwaddch(game_win, y,x,ACS_LLCORNER);
			} else {
				mvwaddch(game_win, y,x,ACS_HLINE);
			}
			x++;
		}

		if(open_bounds_flag) {
			// If you hit the outer bounds you'll end up on the other side
			if(y < 0) {
				y = max_y-1;
			}else if(y >= max_y) {
				y = 0;
			}else if(x < 0) {
				x = max_x-1;
			}else if(x >= max_x){
				x = 0;
			}
		} else {
			// If the bounds aren't open you will loose
			if((y < 0) || (x < 0)) {
				break;
			}
			if((y >= max_y) || (x >= max_x)) {
				break;
			}
		}

		// Draw head and create new cell for the head
		mvwaddch(game_win, y, x, 'X');
		LinkedCell *new_cell = malloc(sizeof(LinkedCell));
		new_cell->x = x;
		new_cell->y = y;
		new_cell->last = head;
		head->next = new_cell;
		head = new_cell;

		// The snake hits itself and dies
		if(is_on_obstacle(head->last, x, y)) {
			break;
		}

		// The snake hits a wall and dies
		if(is_on_obstacle(wall, x, y)) {
			break;
		}

		// Head hits the food
		if((x == food_x) && (y == food_y)) {
			// Let the snake grow and change the speed
			growing += GROW_FACTOR;
			if(speed > MIN_SPEED) {
				speed -= SPEED_FACTOR;
			}
			points += (points_counter + length + (STARTING_SPEED - speed)) * (superfood_counter == 0 ? 5 : 1);
			points_counter = POINTS_COUNTER_VALUE;
			timeout(speed);
			print_points();
			superfood_counter = (superfood_counter == 0) ? SUPERFOOD_COUNTER_VALUE : superfood_counter-1;
			new_random_coordinates(head, wall, &food_x, &food_y);
		}

		// If the snake is not growing...
		if(growing == 0) {
			LinkedCell *new_last;
			// ...delete the last character from the terminal...
			wattrset(game_win, A_NORMAL);
			mvwaddch(game_win, last_cell->y,last_cell->x,' ');
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
		if(points_counter > MIN_points) {
			points_counter--;
		}

		wrefresh(game_win);
	}

	// lost is set to FALSE if the player quit the game
	if(lost) {
		wattrset(status_win, COLOR_PAIR(3) | A_BOLD);
		pause_game("--- YOU LOST ---", 2);
	}

	// Set a new highscore
	if(points > highscore) {
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

	// Delete the screen content
	clear();

	// Delete windows
	delwin(game_win);
	delwin(status_win);

	// If we are repeating we are jumping to the start of the function
	if(repeat) {
		goto round_start;
	}
}

void show_startscreen() {
	int i, key, anchor;
	// Getting screen dimensions
	getmaxyx(stdscr, max_y, max_x);
	// Setting anchor, where the text will start
	anchor = max_y / 4;
	// Clear the whole screen
	clear();
	// Printing logo and instructions
	attrset(COLOR_PAIR(snake_color) | A_BOLD);
	for(i = 0; i<6; i++) {
		print_centered(stdscr, anchor + i, LOGO[i]);
	}
	attrset(COLOR_PAIR(1) | A_BOLD);
	print_centered(stdscr, anchor + 7, "--- (P)lay Game --- (Q)uit --- (C)redits ---");

	// If points != 0 print them to the screen
	if(points != 0) {
		sprintf(txt_buf, "--- Last Score: %lld ---", points);
		print_centered(stdscr, anchor + 8, txt_buf);
	}
	if(highscore != 0) {
		sprintf(txt_buf, "--- Highscore: %lld ---", highscore);
		print_centered(stdscr, anchor + 9, txt_buf);
	}
	// Displaying information on options
	if(open_bounds_flag) {
		print_centered(stdscr, anchor + 10, "--- Playing with open bounds! ---");
	}
	if(wall_flag) {
		print_centered(stdscr, anchor + 11, "--- Walls are activated! ---");
	}
	if(ignore_flag) {
		print_centered(stdscr, anchor + 12, "--- Savefile is ignored! ---");
	} else {
		if(custom_flag && file_path != NULL) {
			print_centered(stdscr, anchor + 12, "Used savefile:");
			print_centered(stdscr, anchor + 13, file_path);
		}
	}

	// Printing verion
	sprintf(txt_buf, "Version: %s", VERSION);
	print_centered(stdscr, max_y - 1, txt_buf);

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
		}else if((key == 'O') || (key == 'o')) {
			open_bounds_flag = !open_bounds_flag;
			break;
		}else if((key == 'C') || (key == 'c')) {
			for(i = 0; i<7; i++) {
				move(anchor + 7 + i, 0);
				clrtoeol();
			}
			print_centered(stdscr, anchor + 7, "--- Programming by Philipp Hagenlocher ---");
			print_centered(stdscr, anchor + 8, "--- Please report any bugs you can find on GitHub ---");
			print_centered(stdscr, anchor + 9, "--- Start with -v to get information on the license ---");
			print_centered(stdscr, anchor + 10, "--- Press any key! ---");
			refresh();
			getch();
			break;
		}
	}
}

void parse_arguments(int argc, char **argv) {
	int arg, color, pattern, option_index = 0;

	static struct option long_opts[] =
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
	};

	while((arg = getopt_long(argc, argv, "osif:rw:c:hv", long_opts, &option_index)) != -1) {
		switch (arg) {
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
				pattern = atoi(optarg);
				if((pattern >= 0) && (pattern <= 5)) {
					wall_flag = TRUE;
					wall_pattern = pattern;
					break;
				}
				goto help_text;
			case 'c':
				color = atoi(optarg);
				if((color >= 1) && (color <= 5)) {
					snake_color = color;
					break;
				} // else pass to help information
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
				printf(" --help, -h\n\tDisplay this information\n");
				printf(" --version, -v\n\tDisplay version and license information\n\n");
				printf("Ingame Controls:\n");
				printf(" Arrow-Keys\n\tDirection to go\n");
				printf(" Enter\n\tPause\n");
				printf(" Shift+Q\n\tEnd Round\n");
				printf(" Shift+R\n\tRestart Round (can be used to resize the game after windowsize has changed)\n");
				exit(0);
			case 'v':
				printf("C-Snake %s\nCopyright (c) 2015-2019 Philipp Hagenlocher\nLicense: MIT\nCheck source for full license text.\nThere is no warranty.\n", VERSION);
				exit(0);
		}
	}
}

int main(int argc, char **argv) {
	// Parse arguments
	parse_arguments(argc, argv);
	// Seed RNG with current time
	srand(time(NULL));
	// Init path for safefile
	init_file_path();
	if(file_path == NULL) {
		ignore_flag = TRUE;
	} else if(remove_flag) {
		remove(file_path);
		exit(0);
	}
	// Read local highscore
	read_score_file();
	// Init colors and ncurses specific functions
	initscr();
	start_color();
	short background = COLOR_BLACK;
	if(use_default_colors() != ERR) {
		background = -1;
	}
	init_pair(1, COLOR_WHITE,  background);
	init_pair(2, COLOR_GREEN,  background);
	init_pair(3, COLOR_RED,    background);
	init_pair(4, COLOR_YELLOW, background);
	init_pair(5, COLOR_BLUE,   background);
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
	if((max_x < 64) || (max_y < 19)) {
		skip_flag = TRUE;
	}
	// 19 chars are used for the max num for a long long. Longest additional
	// string ("--- Last Score: %lld ---") has 20 chars.
	txt_buf = malloc(40);

	// Endless loop until the user quits the game
	while(TRUE) {
		if(skip_flag) {
			play_round();
		} else {
  			show_startscreen();
  		}
  	}

}
