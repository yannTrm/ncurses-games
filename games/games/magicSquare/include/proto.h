void init_board(int **board, int n, tile *blank);
void board(WINDOW *win, int starty, int startx, int lines, int cols, int tile_width, int tile_height);
void shuffle_board(int **board, int n);
void move_blank(int direction, int **s_board, int n, tile *blank);

int check_win(int **s_board, int n, tile *blank);