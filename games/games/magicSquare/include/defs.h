#include <stdlib.h>
#include <time.h>
#include <curses.h>

#define STARTX 9
#define STARTY 3
#define WIDTH  6
#define HEIGHT 4

#define BLANK 0

typedef struct _tile {
	int x;
	int y;
}tile;

enum { LEFT, RIGHT, UP, DOWN };