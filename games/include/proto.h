void welcome();
void print_menu(WINDOW *menu_win, int highlight);

void help(char *game[]);
void play(char *game[]);
void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);
void makeListGame(char *games[],int len);



int getNbFile();
int menu(char *game[]);

void makeTab(char *tab[],int len);