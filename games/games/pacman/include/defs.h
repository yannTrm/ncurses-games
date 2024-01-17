#include <assert.h>
#include <stdint.h>
#include <ncurses.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define internal static
#define global_variable static
#define local_persist static
#define UNUSED(x) (void)(x);
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) < (y) ? (y) : (x))

#define ARRAY_LENGTH(arr)\
	(sizeof(arr) / sizeof(arr[0]))

#define for_array(index, arr)\
	for (index = 0; index < ARRAY_LENGTH(arr); ++index)

#define MS(x) (1000*(x))
#define SECONDS(x) (MS(1000*(x)))

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;
typedef int32_t bool32;
typedef char* c_str;

struct v2 {
	int32 x, y;
};

inline internal struct v2
add(struct v2 a, struct v2 b) {
	a.x += b.x;
	a.y += b.y;
	return a;
}

inline internal struct v2
sub(struct v2 a, struct v2 b) {
	a.x -= b.x;
	a.y -= b.y;
	return a;
}

inline internal bool32
eql(struct v2* a, struct v2* b) {
	return a->x == b->x && a->y == b->y;
}

inline internal struct v2
scale(struct v2 a, int32 amt) {
	a.x *= amt;
	a.y *= amt;
	return a;
}

inline internal int32
floor_div(int32 num, int32 den) {
	double fresult = (double)num / (double)den;

	int32 result = (int32)fresult;
	if (fresult < (double)result) {
		--result;
	}

	return result;
}

inline internal struct v2
div_scale(struct v2 a, int32 amt) {

	a.x = floor_div(a.x, amt);
	a.y = floor_div(a.y, amt);

	return a;
}

uint32
time_in_us() {
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000000 + t.tv_usec;
}

#define SQUARE(x) ((x)*(x))
#define DIST_SQUARE(x, y) (SQUARE(x) + SQUARE(y))

#define PIXEL_SIZE 20 
#define TILE_SIZE_IN_PIXELS 8
#define TILE_SIZE (PIXEL_SIZE * TILE_SIZE_IN_PIXELS)

#define LIFE_LOST_PAUSE_TIME 30
#define LIFE_LOST_TURN_TIME 12

struct v2 TILE_CENTER_IN_PIXELS = { 3, 4 };
struct v2 TILE_CENTER = { 3*PIXEL_SIZE, 4*PIXEL_SIZE };

#define NUM_GHOSTS 4
#define NUM_DIRS 4

struct view {
	int top, left;
	struct v2 camera_target_tile;
	bool32 zoom_view;
};

#define ARENA_WIDTH_IN_TILES 28
#define ARENA_HEIGHT_IN_TILES 31

#define HOUSE_CENTER (ARENA_WIDTH_IN_TILES / 2)
#define HOUSE_LEFT (HOUSE_CENTER - 3)
#define HOUSE_RIGHT (HOUSE_CENTER + 2)
#define HOUSE_TOP 13
#define HOUSE_BOTTOM 15

#define ARENA_HEIGHT (ARENA_HEIGHT_IN_TILES*TILE_SIZE)
#define ARENA_WIDTH (ARENA_WIDTH_IN_TILES*TILE_SIZE)

global_variable
struct v2 top_house_targets[] = {
	{ 0 },
	{ HOUSE_CENTER, HOUSE_TOP },
	{ HOUSE_LEFT, HOUSE_TOP },
	{ HOUSE_RIGHT, HOUSE_TOP },
};
global_variable
struct v2 bottom_house_targets[] = {
	{ 0 },
	{ HOUSE_CENTER, HOUSE_BOTTOM },
	{ HOUSE_LEFT, HOUSE_BOTTOM },
	{ HOUSE_RIGHT, HOUSE_BOTTOM },
};
global_variable
struct v2 forbidden_upward_tiles[] = {
	{ 12, 10 },
	{ 15, 10 },
	{ 12, 22 },
	{ 15, 22 },
};
global_variable
struct v2 fruit_tile = { HOUSE_CENTER, HOUSE_BOTTOM + 2 };


/* Corners:
 * top-left: /
 * top-right: `
 * bottom-left: [
 * bottom-right: ]
 */
const
char arena[ARENA_HEIGHT_IN_TILES*ARENA_WIDTH_IN_TILES] = 
"/------------`/------------`"
"|            ||            |"
"| /--` /---` || /---` /--` |"
"| |xx| |xxx| || |xxx| |xx| |"
"| [--] [---] [] [---] [--] |"
"|                          |"
"| /--` /` /------` /` /--` |"
"| [--] || [--`/--] || [--] |"
"|      ||    ||    ||      |"
"[----` |[--` || /--]| /----]"
"xxxxx| |/--] [] [--`| |xxxxx"
"xxxxx| ||          || |xxxxx"
"xxxxx| || +--__--+ || |xxxxx"
"-----] [] |xxxxxx| [] [-----"
"          |xxxxxx|          "
"-----` /` |xxxxxx| /` /-----"
"xxxxx| || +------+ || |xxxxx"
"xxxxx| ||          || |xxxxx"
"xxxxx| || /------` || |xxxxx"
"/----] [] [--`/--] [] [----`"
"|            ||            |"
"| /--` /---` || /---` /--` |"
"| [-`| [---] [] [---] |/-] |"
"|   ||                ||   |"
"[-` || /` /------` /` || /-]"
"/-] [] || [--`/--] || [] [-`"
"|      ||    ||    ||      |"
"| /----][--` || /--][----` |"
"| [--------] [] [--------] |"
"|                          |"
"[--------------------------]"
;

const
char dot_placement_map[ARENA_HEIGHT_IN_TILES*ARENA_WIDTH_IN_TILES] = 
"+------------++------------+"
"|............||............|"
"|.+--+.+---+.||.+---+.+--+.|"
"|*|  |.|   |.||.|   |.|  |*|"
"|.+--+.+---+.++.+---+.+--+.|"
"|..........................|"
"|.+--+.++.+------+.++.+--+.|"
"|.+--+.||.+--++--+.||.+--+.|"
"|......||....||....||......|"
"+----+.|+--+ || +--+|.+----+"
"     |.|+--+ ++ +--+|.|     "
"     |.||          ||.|     "
"     |.|| +--__--+ ||.|     "
"-----+.++ |      | ++.+-----"
"      .   |      |   .      "
"-----+.++ |      | ++.+-----"
"     |.|| +------+ ||.|     "
"     |.||          ||.|     "
"     |.|| +------+ ||.|     "
"+----+.++ +--++--+ ++.+----+"
"|............||............|"
"|.+--+.+---+.||.+---+.+--+.|"
"|.+-+|.+---+.++.+---+.|+-+.|"
"|*..||.......  .......||..*|"
"+-+.||.++.+------+.++.||.+-+"
"+-+.++.||.+--++--+.||.++.+-+"
"|......||....||....||......|"
"|.+----++--+.||.+--++----+.|"
"|.+--------+.++.+--------+.|"
"|..........................|"
"+--------------------------+"
;

char dot_map[ARENA_HEIGHT_IN_TILES][ARENA_WIDTH_IN_TILES];

enum {
	BG_PAIR = 1,
	PACMAN_PAIR,
	DOT_PAIR,
	GAME_OVER_TEXT_PAIR,
	BLINKY_PAIR,
	INKY_PAIR,
	PINKY_PAIR,
	CLYDE_PAIR,
	FRIGHT_PAIR,
	FRIGHT_FLASH_PAIR,
	EYES_PAIR,
	EMPTY_PAIR,
	WALL_PAIR,
	DOOR_PAIR,

	CHERRIES_PAIR,
	STRAWBERRY_PAIR,
	PEACH_PAIR,
	APPLE_PAIR,
	GRAPES_PAIR,
	GALAXIAN_PAIR,
	BELL_PAIR,
	KEY_PAIR,
};

internal char
arena_get(int row, int col) {
	assert(row >= 0 && row < ARENA_HEIGHT_IN_TILES);
	if (col < 0) {
		col += ARENA_WIDTH_IN_TILES;
	} else if (col >= ARENA_WIDTH_IN_TILES) {
		col -= ARENA_WIDTH_IN_TILES;
	}
	return arena[row*ARENA_WIDTH_IN_TILES + col];
}

enum dir {
	UP,
	LEFT,
	DOWN,
	RIGHT
};

#define DRAW_SIZE 8

internal struct v2
draw_pos(int row, int col, struct view* view) {
	struct v2 ret = {
		3 * (col - view->camera_target_tile.x) + view->left,
		(row - view->camera_target_tile.y) + view->top,
	};
	return ret;
}

internal struct v2
draw_pos_v2(struct v2 p, struct view* view) {
	return draw_pos(p.y, p.x, view);
}

internal void
draw_tile(int row, int col, char ch, struct view* view, char fill_ch) {
	if (view->zoom_view) {
		int tile_size = DRAW_SIZE;
		int i;
		for (i = 0; i < tile_size; ++i) {
			int j;
			for (j = 0; j < tile_size; ++j) {
				mvaddch(tile_size * (row - view->camera_target_tile.y) + view->top + j, tile_size * (col - view->camera_target_tile.x) + view->left + i, fill_ch);
			}
		}
	} else {
		struct v2 pos = draw_pos(row, col, view);
		mvaddch(pos.y, pos.x, fill_ch);
		mvaddch(pos.y, pos.x + 1, ch);
		mvaddch(pos.y, pos.x + 2, fill_ch);
	}
}

internal void
draw_tile_v2(struct v2 tile_pos, char ch, struct view* view, char fill_ch) {
	draw_tile(tile_pos.y, tile_pos.x, ch, view, fill_ch);
}

enum {
	COLOR_LIGHT_BLUE=51,
	COLOR_PINK=197,
	COLOR_ORANGE=172,
	COLOR_DARK_RED=88,
	COLOR_BRIGHT_RED=196,
	COLOR_PURPLE=91,
	COLOR_DOOR_RED=124,
	COLOR_PACMAN_YELLOW=226,
	COLOR_WALL_BLUE=21,
	COLOR_DARK_BACKGROUND=232,
	COLOR_GREY=233,
};

const char* DIR_NAMES[] = {
	"Up", "Down", "Left", "Right"
};

internal bool32
is_h_dir(enum dir dir) {
	return dir == LEFT || dir == RIGHT;
}

internal void
move_in_dir(enum dir dir, struct v2* pos, int amt) {
	switch (dir) {
		case RIGHT:
			pos->x += amt;
			break;
		case LEFT:
			pos->x -= amt;
			break;
		case UP:
			pos->y -= amt;
			break;
		case DOWN:
			pos->y += amt;
			break;
	}
}

internal struct v2
pos_to_tile(struct v2* p) {
	return div_scale(*p, TILE_SIZE);
}

int num_blocked_tiles = 0;
struct v2 blocked_tiles[20];

enum ghost_mode {
	NORMAL,
	FRIGHTENED,
	EYES,
};

internal bool32
test_pacman_dir_blocked(enum dir dir, struct v2* next_pos) {
	int mv_amt = TILE_SIZE / 2;
	if (dir == RIGHT || dir == UP) {
		mv_amt += PIXEL_SIZE;
	}
	move_in_dir(dir, next_pos, mv_amt);
	struct v2 next_tile = pos_to_tile(next_pos);
	return !!strchr("/`[]-|+_", arena_get(next_tile.y, next_tile.x));
}

inline internal uint32
get_speed(int percentage) {
	return percentage / (100 / PIXEL_SIZE);
}

/* TODO
 *
 * BUG: Ghosts get stuck!
 * Try:
 * start from every position, and make sure inky can get out.
 * 
 * onomatopoeic sound effect?
 * intro screen?
 *
 */

enum bonus_symbol {
	CHERRIES,
	STRAWBERRY,
	PEACH,
	APPLE,
	GRAPES,
	GALAXIAN,
	BELL,
	KEY
};

struct level_constants {
	enum bonus_symbol bonus_symbol;
	uint32 pacman_speed, pacman_powerup_speed;
	uint32 ghost_speed, ghost_tunnel_speed, ghost_frightened_speed;
	uint32 num_ghost_flashes;
	uint32 pacman_powerup_time;
	uint32 elroy_v1_dots_left, elroy_v1_speed;

	uint32 scatter_times[4];
	uint32 chase_times[3];

	uint32 ghost_dot_limits[NUM_GHOSTS];
};

inline internal uint32
seconds_to_frames(uint32 seconds) {
	return SECONDS(seconds) / MS(16);
}

enum ghost_name {
	BLINKY,
	PINKY,
	INKY,
	CLYDE
};

uint32 symbol_points[] = {
	100, 300, 500, 700, 1000, 2000, 3000, 5000
};

internal enum bonus_symbol
get_symbol_for_level(uint32 level) {
	enum bonus_symbol level_symbols[] = {
		CHERRIES, STRAWBERRY, PEACH, PEACH, APPLE, APPLE, GRAPES, GRAPES,
		GALAXIAN, GALAXIAN, BELL, BELL
	};
	if (level < ARRAY_LENGTH(level_symbols)) {
		return level_symbols[level];
	} else {
		return KEY;
	}
}

internal void
set_level_constants(struct level_constants* level_constants, uint32 level) {
	level_constants->bonus_symbol = get_symbol_for_level(level);

	if (level == 0) {
		level_constants->pacman_speed = get_speed(80);
		level_constants->pacman_powerup_speed = get_speed(90);
	} else if (level < 4 || level >= 20) {
		level_constants->pacman_speed = get_speed(90);
		level_constants->pacman_powerup_speed = get_speed(95);
	} else {
		level_constants->pacman_powerup_speed = level_constants->pacman_speed = get_speed(100);
	}

	if (level == 0) {
		level_constants->ghost_speed = get_speed(75);
		level_constants->ghost_tunnel_speed = get_speed(40);
		level_constants->ghost_frightened_speed = get_speed(50);
	} else if (level < 4) {
		level_constants->ghost_speed = get_speed(85);
		level_constants->ghost_tunnel_speed = get_speed(45);
		level_constants->ghost_frightened_speed = get_speed(55);
	} else {
		level_constants->ghost_speed = get_speed(95);
		level_constants->ghost_tunnel_speed = get_speed(50);
		level_constants->ghost_frightened_speed = get_speed(60);
	}

	if ((level != 8 && level <= 10) || level == 14) {
		level_constants->num_ghost_flashes = 5;
	} else {
		level_constants->num_ghost_flashes = 3;
	}

	uint32 pacman_powerup_times[] = {
		6, 5, 4, 3, 2, 5, 2, 2, 1, 5, 2, 1, 1, 3, 1, 1, 0, 1
	};
	if (level == 16 || level >= ARRAY_LENGTH(pacman_powerup_times)) {
		level_constants->pacman_powerup_time = 1;
	} else {
		level_constants->pacman_powerup_time = seconds_to_frames(pacman_powerup_times[level]);
	}

	level_constants->elroy_v1_speed = get_speed(100);
	if (level == 0) {
		level_constants->elroy_v1_dots_left = 20;
		level_constants->elroy_v1_speed = get_speed(80);
	} else if (level == 1) {
		level_constants->elroy_v1_dots_left = 30;
		level_constants->elroy_v1_speed = get_speed(90);
	} else if (level < 5) {
		level_constants->elroy_v1_dots_left = 40;
	} else if (level < 8) {
		level_constants->elroy_v1_dots_left = 50;
	} else if (level < 11) {
		level_constants->elroy_v1_dots_left = 60;
	} else if (level < 14) {
		level_constants->elroy_v1_dots_left = 80;
	} else if (level < 18) {
		level_constants->elroy_v1_dots_left = 100;
	} else {
		level_constants->elroy_v1_dots_left = 120;
	}

	level_constants->chase_times[0] = level_constants->chase_times[1] = seconds_to_frames(20);
	if (level == 0) {
		level_constants->scatter_times[0] = level_constants->scatter_times[1] = seconds_to_frames(7);
		level_constants->scatter_times[2] = level_constants->scatter_times[3] = seconds_to_frames(5);
		level_constants->chase_times[2] = seconds_to_frames(20);
	} else if (level < 4) {
		level_constants->scatter_times[0] = level_constants->scatter_times[1] = seconds_to_frames(7);
		level_constants->scatter_times[2] = seconds_to_frames(5);
		level_constants->scatter_times[3] = 1;
		level_constants->chase_times[2] = seconds_to_frames(1033);
	} else {
		level_constants->scatter_times[0] = level_constants->scatter_times[1] = level_constants->scatter_times[2] = seconds_to_frames(5);
		level_constants->scatter_times[3] = 1;
		level_constants->chase_times[2] = seconds_to_frames(1037);
	}

	level_constants->ghost_dot_limits[BLINKY] = 0;
	level_constants->ghost_dot_limits[PINKY] = 0;
	if (level == 0) {
		level_constants->ghost_dot_limits[INKY] = 30;
		level_constants->ghost_dot_limits[CLYDE] = 60;
	} else if (level == 1) {
		level_constants->ghost_dot_limits[INKY] = 0;
		level_constants->ghost_dot_limits[CLYDE] = 50;
	} else {
		level_constants->ghost_dot_limits[INKY] = 0;
		level_constants->ghost_dot_limits[CLYDE] = 0;
	}
}

enum ghost_house_state {
	EXITING_GHOST_HOUSE,
	OUTSIDE_GHOST_HOUSE,
	ENTERING_GHOST_HOUSE,
	IN_GHOST_HOUSE,
};

struct ghost {
	struct v2 pos;
	enum dir dir;
	struct v2 last_tile;

	bool32 is_path_chosen;
	enum dir chosen_dir;

	struct v2 target_tile;
	enum ghost_mode mode;
	enum ghost_name name;
	char nickname;
	int curses_color_pair;

	enum ghost_house_state ghost_house_state;

	uint32 dot_counter;

	bool32 bounce_target_bottom;
};

enum game_mode {
	PAUSED_BEFORE_PLAYING,
	PLAYING,
	LOSING_A_LIFE,
	GAME_OVER,
	LEVEL_TRANSITION,
};

#define TUNNEL_WIDTH (4*TILE_SIZE)
#define GHOST_FLASH_TIME 10

internal void
init_ghost(struct ghost* ghost_in, char nickname, enum ghost_name name, int curses_color_pair) {
	struct ghost ghost;
	memset(&ghost, 0, sizeof(ghost));
	ghost.nickname = nickname;
	ghost.name = name;
	ghost.curses_color_pair = curses_color_pair;

	*ghost_in = ghost;
}

internal void
reverse_ghosts(struct ghost* ghosts) {
	int i;
	for (i = 0; i < NUM_GHOSTS; ++i) {
		ghosts[i].is_path_chosen = TRUE;
		if (ghosts[i].mode == NORMAL && ghosts[i].ghost_house_state == OUTSIDE_GHOST_HOUSE) {
			switch (ghosts[i].dir) {
				case UP:
					ghosts[i].chosen_dir = DOWN;
					break;
				case DOWN:
					ghosts[i].chosen_dir = UP;
					break;

				case LEFT:
					ghosts[i].chosen_dir = RIGHT;
					break;
				case RIGHT:
					ghosts[i].chosen_dir = LEFT;
					break;
			}
		}
	}
}

const
enum dir ghost_start_dirs[NUM_GHOSTS] = {
	LEFT, UP, UP, UP
};

#define NUM_DOTS 244

struct level_data {
	struct ghost* ghost_to_be_eaten;
	uint32 pacman_chomp_timer, pacman_eat_timer,
				 ghost_eat_timer, pacman_powerup_timer,
				 ghost_mode_timer, fruit_timer,
				 ghost_flash_timer, num_ghost_mode_cycles,
				 fruit_score_timer, consecutive_ghosts_eaten,
				 extra_life_timer;
	uint32 fright_ghost_seed;
	uint32 global_ghost_house_dot_counter;
	bool32 fruit_is_visible, pacman_blocked, pacman_turning, is_chasing, use_global_ghost_house_dot_counter;
	uint32 dots_eaten;

	struct v2 pacman_pos;
	enum dir pacman_dir, next_dir;
	uint32 dot_timer;
};

internal void
get_normal_target_tile(
		struct level_data* level_data,
		struct v2 scatter_targets[NUM_GHOSTS],
		struct v2* blinky_pos, struct ghost* ghost, struct v2 ghost_tile) {
	struct v2 pacman_tile = pos_to_tile(&level_data->pacman_pos);
	/* Chase after a target */
	if (level_data->is_chasing) {
		switch (ghost->name) {
			case BLINKY: {
				/* Follow pacman */
				ghost->target_tile = pacman_tile;
			} break;
			case PINKY: {
				/* Follow pacmans direction of motion. */
				struct v2 target = pacman_tile;
				move_in_dir(level_data->pacman_dir, &target, 4);
				ghost->target_tile = target;
			} break;
			case INKY: {
				/* This gets close sometimes, but usually runs away. */
				struct v2 blinky_tile = pos_to_tile(blinky_pos);
				struct v2 target = pacman_tile;
				move_in_dir(level_data->pacman_dir, &target, 2);
				struct v2 delta = sub(target, blinky_tile);
				target = add(target, delta);
				ghost->target_tile = target;
			} break;
			case CLYDE: {
				struct v2 target = pacman_tile;
				struct v2 delta = sub(ghost_tile, target);
				int dist_square = DIST_SQUARE(delta.x, delta.y);
				/* Follow pacman from a distance */
				if (dist_square >= SQUARE(8)) {
					ghost->target_tile = target;
				} else {
					/* Scatter if too close. */
					ghost->target_tile = scatter_targets[CLYDE];
				}
			} break;
		}
	} else {
		ghost->target_tile = scatter_targets[ghost->name];
	}
}

internal void 
return_ghosts_and_pacman_to_start_position(struct ghost ghosts[NUM_GHOSTS], struct level_data* level_data) {
	struct v2 pacman_start_pos = { ARENA_WIDTH / 2, (ARENA_HEIGHT_IN_TILES - 8)*TILE_SIZE + TILE_CENTER.y };

	int i;
	for (i = 0; i < NUM_GHOSTS; ++i) {
		struct v2 start_position;
		switch (i) {
			case BLINKY: {
				struct v2 blinky_start_pos_s = { ARENA_WIDTH / 2 - 2*PIXEL_SIZE, 11*TILE_SIZE + TILE_CENTER.y };
				start_position = blinky_start_pos_s;
			} break;
			case INKY:
				start_position = add(TILE_CENTER, scale(bottom_house_targets[INKY], TILE_SIZE));
				break;
			case PINKY:
				start_position = add(TILE_CENTER, scale(bottom_house_targets[PINKY], TILE_SIZE));
				break;
			case CLYDE:
				start_position = add(TILE_CENTER, scale(bottom_house_targets[CLYDE], TILE_SIZE));
				break;
		}

		ghosts[i].pos = start_position;
		ghosts[i].last_tile = pos_to_tile(&ghosts[i].pos);
		ghosts[i].dir = ghost_start_dirs[i];

		ghosts[i].is_path_chosen = FALSE;
		ghosts[i].mode = NORMAL;
		ghosts[i].bounce_target_bottom = FALSE;

		if (i == BLINKY) {
			ghosts[i].ghost_house_state = OUTSIDE_GHOST_HOUSE;
		} else {
			ghosts[i].ghost_house_state = IN_GHOST_HOUSE;
		}
	}
	level_data->pacman_pos = pacman_start_pos;
	level_data->pacman_dir = LEFT;
	level_data->pacman_blocked = FALSE;
	level_data->pacman_turning = FALSE;
	level_data->next_dir = level_data->pacman_dir;
	level_data->dot_timer = 0;
}

internal void
start_new_level(struct ghost ghosts[NUM_GHOSTS], struct level_data* level_data, uint32 original_fright_ghost_seed) {
	int ghost;
	for (ghost = 0; ghost < NUM_GHOSTS; ++ghost) {
		ghosts[ghost].dot_counter = 0;
	}

	int row, col;
	for (row = 0; row < ARENA_HEIGHT_IN_TILES; ++ row) {
		for (col = 0; col < ARENA_WIDTH_IN_TILES; ++ col) {
			char ch = dot_placement_map[row*ARENA_WIDTH_IN_TILES + col];
			if (ch == '.' || ch == '*') {
				dot_map[row][col] = ch;
			} else {
				dot_map[row][col] = '\0';
			}
		}
	}
	memset(level_data, 0, sizeof(struct level_data));
	level_data->fright_ghost_seed = original_fright_ghost_seed;
	level_data->ghost_mode_timer = 0;

	return_ghosts_and_pacman_to_start_position(ghosts, level_data);
}

struct game_data {
	uint32 num_extra_lives;
	uint32 current_level;
	uint32 score;
	uint32 original_fright_ghost_seed;
};

internal void
start_new_game(struct game_data* game_data, struct ghost ghosts[NUM_GHOSTS], struct level_data* level_data, struct level_constants* level_constants) {
	game_data->num_extra_lives = 2;
	game_data->current_level = 0;
	game_data->score = 0;
	game_data->original_fright_ghost_seed = time(NULL);
	set_level_constants(level_constants, game_data->current_level);

	init_ghost(&ghosts[BLINKY], 'B', BLINKY, BLINKY_PAIR);
	init_ghost(&ghosts[INKY], 'I', INKY, INKY_PAIR);
	init_ghost(&ghosts[PINKY], 'P', PINKY, PINKY_PAIR);
	init_ghost(&ghosts[CLYDE], 'C', CLYDE, CLYDE_PAIR);

	start_new_level(ghosts, level_data, game_data->original_fright_ghost_seed);
}

internal void
enable_bonus_symbol_color(enum bonus_symbol bonus_symbol) {
	attron(COLOR_PAIR(bonus_symbol + CHERRIES_PAIR));
}

internal void
draw_fruit(struct v2 left_tile, enum bonus_symbol bonus_symbol, struct view* view) {
	const char* bonus_strs[] = {
		"^00",
		"<0'",
		"(0)",
		"`0)",
		"`88",
		"|^|",
		"3>-",
		"3-^",
	};
	enable_bonus_symbol_color(bonus_symbol);
	struct v2 pos = draw_pos_v2(left_tile, view);
	mvprintw(pos.y, pos.x, bonus_strs[bonus_symbol]);
}

internal uint32
get_ghost_score(uint32 ghosts_eaten) {
	return (2 << ghosts_eaten) * 100;
}

internal void
add_extra_life(struct game_data* game_data, struct level_data* level_data) {
	level_data->extra_life_timer = seconds_to_frames(1);
	if (game_data->num_extra_lives < 5) {
		++game_data->num_extra_lives;
	}
}