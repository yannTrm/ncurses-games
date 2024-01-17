#include <defs.h>
#include <proto.h>

int wbox = 0;
int lev = 0;

int main(){
    int ch;
    initscr();  // creation fenetre
    cbreak();  // inteprète les touches
    curs_set(FALSE); 
    keypad(stdscr, TRUE);  // importante
    //keypad(true);
    if (!has_colors())  // si le terminal ne permet pas la prise en compte des couleurs
    {
        endwin();
    }
    start_color();
    color();
    level(lev);
    while((ch = getch()) != 'q'){
        play(ch);
    }
    endwin();
    return 0;
}


void levList(int *h, int *w, int *array, int y, int x, int n){
    // creation des maps
    if (n == 0){
        *h = 7;
        *w = 6;
        int map0[7][6] = {
            {1,1,1,1,1,1},
            {1,0,2,1,1,1},
            {1,0,0,1,1,1},
            {1,2,5,0,0,1},
            {1,4,0,4,0,1},
            {1,0,0,1,1,1},
            {1,1,1,1,1,1}
        };
        *array = map0[y][x];
    }
    else if (n == 1)
    {
        *h = 9;
        *w = 11;
        int map1[9][10] = {
            {1,1,1,1,1,1,1,1,1,1},
            {1,0,0,0,1,1,1,1,1,1},
            {1,0,1,0,1,0,2,1,1,1},
            {1,0,0,0,0,4,0,1,1,1},
            {1,1,1,0,1,4,2,0,0,1},
            {1,0,0,0,1,5,0,0,0,1},
            {1,0,1,0,1,1,1,1,1,1},
            {1,0,0,0,1,1,1,1,1,1},
            {1,1,1,1,1,1,1,1,1,1}
        };
        *array = map1[y][x];
    }
    else if (n == 2){
        *h = 9;
        *w = 11;
        int map2[9][11] = {
            {0,0,0,0,0,1,1,1,1,1,1},
            {0,0,0,0,0,1,0,0,0,1,1},
            {0,0,0,0,0,1,0,0,0,0,1},
            {0,1,1,1,1,1,1,0,0,0,1},
            {1,1,0,0,0,0,0,1,2,0,1},
            {1,0,4,0,4,0,5,0,0,1,1},
            {1,0,1,1,1,1,1,1,2,1,1},
            {1,0,0,0,0,0,0,0,0,1,1},
            {1,1,1,1,1,1,1,1,1,1,1}
        };
        *array = map2[y][x];
    }
}

void color(){
    init_color(COLOR_BLACK, 0, 0, 0);
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_CYAN, COLOR_BLACK);
}

void level(int n){
    clear(); // on efface la fenetre
    mvprintw(2,1, "Restart-R, Map-M, Exit-Q");
    int x = 0, y = 0, h = 1, w = 1, map;
    wbox = 0;
    for (y = 0; y < h; y++){
        for (x = 0; x < w; x++){
            levList(&h,&w,&map,y,x,n);
            switch (map)
            {
            case 0 : mvaddch(y+4,x+10,'-' | COLOR_PAIR(4)); break;
            case 1 : mvaddch(y+4,x+10,'@' | COLOR_PAIR(1)); break;
            case 2 : mvaddch(y+4,x+10,'x' | COLOR_PAIR(2)); break;
            case 4 :
                mvaddch(y+4,x+10,'-' | COLOR_PAIR(4));
                wbox +=1;
                obj[wbox].ozn = mvinch(y+4,x+10);
                obj[wbox].y = y+4;
                obj[wbox].x = x + 10;
                obj[wbox].zn = '@';
                mvaddch(obj[wbox].y, obj[wbox].x, obj[wbox].zn | COLOR_PAIR(5));
                break;
            case 5:
                mvaddch(y+4,x+10,'-' | COLOR_PAIR(4));
                wbox +=1;
                obj[0].ozn = mvinch(y+4,x+10);
                obj[0].y = y+4;
                obj[0].x = x + 10;
                obj[0].zn = 'P';
                mvaddch(obj[wbox].y, obj[wbox].x, obj[wbox].zn | COLOR_PAIR(3));
                break;
            }
        }
    }
    move(obj[0].y, obj[0].x);
}

void play(int input){
    bool restart = FALSE;
    chtype up, lf, dw, rg, oup, olf, odw, org;
    up = (mvinch(obj[0].y - 1, obj[0].x) & A_CHARTEXT);
    lf = (mvinch(obj[0].y , obj[0].x-1) & A_CHARTEXT);
    dw = (mvinch(obj[0].y + 1, obj[0].x) & A_CHARTEXT);
    rg = (mvinch(obj[0].y , obj[0].x+1) & A_CHARTEXT);
    oup = (mvinch(obj[0].y - 2, obj[0].x) & A_CHARTEXT);
    olf = (mvinch(obj[0].y, obj[0].x-2) & A_CHARTEXT);
    odw = (mvinch(obj[0].y +2, obj[0].x) & A_CHARTEXT);
    org = (mvinch(obj[0].y , obj[0].x+2) & A_CHARTEXT);

    for (int o = 0; o <= wbox; o++){
        mvaddch(obj[o].y, obj[o].x, obj[o].ozn);
    }
    switch (input)
    { 
    case KEY_UP:
        if (up != 35)
        {
            if (up == 64 && (oup == 45 || oup == 120)){
                obj[0].y -= 1;
                for (int o = 1; o <= wbox; o++){
                    if ((obj[0].y == obj[o].y) && (obj[0].x == obj[o].x)){
                        obj[o].y -= 1;
                    }
                }
            }
            else if (up != 64) obj[0].y -= 1;
        }
        break;
    case KEY_DOWN:
        if (dw != 35){
            if (dw == 64 && (odw == 45 || odw == 120)){
                obj[0].y += 1;
                for (int o = 1; o <= wbox; o++){
                    if ((obj[0].y == obj[o].y) && (obj[0].x == obj[o].x)){
                        obj[o].y += 1;
                    }
                }
            }
            else if (dw != 64) obj[0].y += 1 ;
        }
        break;
    case KEY_LEFT:
        if (lf != 35){
            if (lf == 64 && (olf == 45 || olf == 120)){
                obj[0].x -= 1;
                for (int o = 1; o <= wbox; o++){
                    if ((obj[0].y == obj[o].y) && (obj[0].x == obj[o].x)){
                        obj[o].x -= 1;
                    }
                }
            }
            else if (lf != 64) obj[0].x -= 1 ;
        }
        break;
    case KEY_RIGHT:
        if (rg != 35){
            if (rg == 64 && (org == 45 || org == 120)){
                obj[0].x += 1;
                for (int o = 1; o <= wbox; o++){
                    if ((obj[0].y == obj[o].y) && (obj[0].x == obj[o].x)){
                        obj[o].x += 1;
                    }
                }
            }
            else if (rg != 64) obj[0].x += 1 ;
        }
        break;
    case 'm':
    case 'M':
        restart = TRUE;
        if (lev < 2) lev +=1;
        else lev = 0;
        level(lev);
        break;
    case 'r':
    case 'R':
        restart = TRUE;
        level(lev);
        break;
    default:
        break;
    }
    if (!restart){
        for (int o = 0; o <= wbox; o++){
            obj[o].ozn = mvinch(obj[o].y, obj[o].x);
            mvaddch(obj[o].y, obj[o].x, obj[o].zn | ((o==0) ? COLOR_PAIR(3) : COLOR_PAIR(5)));
        }
        move(obj[0].y, obj[0].x);
    }
    else restart = FALSE;
} 
