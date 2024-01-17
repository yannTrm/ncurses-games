#include <stdlib.h>
#include <curses.h>

#define CELL_CHAR '¤'
#define TIME_OUT  300



typedef struct _state {
	int oldstate;
	int newstate;
}state;


int STARTX = 0;
int STARTY = 0;
int ENDX = 79;
int ENDY = 24;