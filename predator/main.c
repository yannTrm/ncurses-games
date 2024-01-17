// gcc -o main main.c -lncurses
// ./main
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>

const int GRASS_GROWTH = 1;

int mod(int x, int n){
    int r = x % n;
    return r >= 0 ? r : -r;
}

typedef enum Species{
    student,prof
} Species;

typedef struct Agent {
    int x, y;
    int energy;
    Species species;
    int time_last_moved;
} Agent;

Agent *init_Agent(int x, int y, Species species){
    Agent *agent  = calloc(1,sizeof(Agent));
    agent->x = x; 
    agent->y = y;
    agent->energy = 0;
    agent->species = species;
    agent->time_last_moved = -1;

    return agent; 
}


typedef struct Cell{
    int y,x;
    int grass;
    Agent *agent;
} Cell;

Cell *init_Cell(int y, int x){
    Cell *cell = calloc(1, sizeof(Cell));
    cell->y = y;
    cell->x = x;
    cell->grass = 0;
    cell->agent = NULL;
}

typedef struct Board{
    int rows,cols;
    Cell **cells;
} Board;

int coords_to_ind(Board * board, int y, int x){
    int r = board->rows;
    int c = board->cols;
    return mod(y,r) * c + mod(x,c);
}

Board *init_Board(int rows, int cols){
    Board *board = calloc(1, sizeof(Board));
    board->rows = rows;
    board->cols = cols;
    board->cells = calloc(rows * cols, sizeof(Cell*));

    int i,j;
    for (i=0; i < rows ; i++){
        for (j = 0; j< cols; j++){
            Cell *cell = init_Cell(i,j);
            board->cells[coords_to_ind(board,i,j)] = cell;
        }
    }
    return board;
}

Cell *board_at(Board *board, int y, int x){
    return board->cells[coords_to_ind(board,y,x)];
}

char cell_to_char(Cell *cell){
    if (cell->agent == NULL){
        return ' ';
    }
    Species s = cell->agent->species;
    switch(s){
        case student:
            return '0';
            break;
        case prof:
            return '1';
            break; 
    }
}

void print_board(Board *board){
    int i,j;
    for (i = 0; i < board->rows ; i++){
        for (j=0; j < board->cols; j++){
            Cell *cell = board_at(board,i,j);
            char ch = cell_to_char(cell);
            mvprintw(i,j,"%c",ch);
        }
    }
}


void populate_board(Board *board){
    int cols = board->cols;
    int rows = board->rows;

    int i,j;
    for(i = 0; i < rows; i++){
        for (j = 0; j < cols ; j++){
            int spawnagent = rand() % 10;
            if(spawnagent == 0){
                int whichAgent = rand() % 10;
                if (whichAgent == 0){
                    Species s = 1;
                    Agent *agent = init_Agent(i,j,s);
                    agent->energy = 5;
                    board->cells[coords_to_ind(board,i,j)]->agent = agent;
                }else{
                    Species s = 0;
                    Agent *agent = init_Agent(i,j,s);
                    agent->energy = 5;
                    board->cells[coords_to_ind(board,i,j)]->agent = agent;
                }
                
            }

        } 
    }
} 


Cell *get_adjacent(Board *board, int y, int x){
    int dy,dx;
    int numempty = 0;
    Cell *empty[9];
    for (dy = -1; dy < 2; dy++){
        for (dx = -1; dx < 2; dx++){
            Cell *consider = board_at(board,y+dy,x+dx);
            if(consider->agent != NULL){
                empty[numempty] = consider;
                numempty++;
            }
        }
    }
}

void board_update(Board *board, int time){
    int i,j;
    int rows = board->rows;
    int cols = board->cols;

    for (i = 0; i < rows; i++){
        for (j=0; j< cols; j++){
            Cell *cell = board_at(board,i,j);
            cell->grass += GRASS_GROWTH;

            Agent *agent = cell->agent;
            if (agent == NULL) continue;
            if (agent->time_last_moved >= time)continue;
            agent->time_last_moved = time;

            //agent->energy -=3;
            if (agent->energy <=0){
                cell->agent = NULL;
                free(agent);
                continue;
            }

            int dy = mod(rand() , 3) - 1;
            int dx = mod(rand() , 3) - 1;

            Cell *dcell = board_at(board, agent->y +dy, agent->x +dx);
            if (dcell->agent == NULL){
                dcell->agent = agent;
                cell->agent = NULL;

                agent->y = dcell->y;
                agent->x = dcell->x;
            }
            agent->energy -= 1;

            int fx,fy;
            switch (agent->species)
            {
            case student:
                agent->energy += cell->grass;
                cell->grass=0;
                break;
            case prof:
                fx = agent->x;
                fy= agent->y;
                for (int dy = -1; dy < 2; dy++){
                    for (int dx = -1; dx < 2; dx++){
                        Cell *lookcell = board_at(board,fy +dy, fx +dx);
                        Agent *prey = lookcell->agent;
                        if (prey !=NULL && prey->species == student){
                            agent->energy += prey->energy/3;
                            prey->energy = 0;
                            free(prey);
                            lookcell->agent = NULL;
                        }
                    }
                }
                break;
            }

            if (agent->energy >= 10){
                Cell *bcell = get_adjacent(board,agent->y,agent->x);
                if(bcell !=NULL){
                    Agent *child = init_Agent(cell->y,cell->x,agent->species);
                    cell->agent = child;
                    child->energy = 5;
                    agent->energy -=5;
                }
            }
        }
    }
}

int main(){

    initscr();
    noecho();

    srand(time(NULL));
    int rows = 50;
    int cols = 50;

    Board *board = init_Board(rows,cols);
    populate_board(board);
    print_board(board);
    refresh();

    int time =0;

    while(++time){
        board_update(board, time);
        print_board(board);
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 10000 * 100;
        nanosleep(&ts,&ts);
        refresh();
    }

    getchar();
    endwin();
}