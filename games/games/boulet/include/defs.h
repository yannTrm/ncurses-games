#include <ncurses.h>
#include <stdlib.h>


struct Object{
    int x;
    int y;
    unsigned char zn;
    chtype ozn;
};

#define N 10
struct Object obj[N] = {};
