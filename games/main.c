#include <defs.h>
#include <proto.h>

int main(void)
{ 
	int len = getNbFile() +1;      /* récupère les noms de dossier dans games */
	char *game[len];
	makeTab(game,len);
	game[len-1] = (char *) NULL;

	initscr();                      /* initialize NCurses */
	clear();
	noecho();
	cbreak();
  
	welcome();
	menu(game);
    endwin();                       /* clean up NCurses */
 
    return 0;                       
}


int getNbFile()
{
	struct dirent *dir;
    	// opendir() renvoie un pointeur de type DIR. 
	int c = 0;
    DIR *d = opendir("./games"); 
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
        	c+=1;
		}
        closedir(d);
    }
    return c;  // retourne le nombre de repertoire dans le dosssier games
}

void makeTab(char *tab[], int len){  //permet de récuperer dans un tableau les noms de dossier des jeux
	struct dirent *dir;
	DIR *d = opendir("./games");
	if(d){
		int c = 0;
		while ((dir = readdir(d)) != NULL){
			tab[c] = dir->d_name;
			c++;
		}
	}	
}

void welcome()
{
	int keyInput;
	char welcome[] = "Welcome dear SuSu ! We hope you will enjoy these games";
	char pressAnyKey[] = "Please press c to continue";
    char *t;
	char *p;
    int row,col;
    getmaxyx(stdscr,row,col);
    start_color();
    init_pair(1,COLOR_WHITE,COLOR_MAGENTA);
    bkgd(COLOR_PAIR(1));
    t = welcome;                       /* initialize the pointer */
	p = pressAnyKey;
    move(row/2,(col-strlen(welcome))/2);
    while(*t)                       /* loop through the whole string */
    {
        addch(*t);              /* put one char to curscr */
        t++;                    /* increment the pointer */
        refresh();              /* update the screen */
        napms(20);             /* delay a bit to see the display */
	}                               /* end while */
	napms(60);
	move((row/2)+1,(col-strlen(pressAnyKey))/2);
	while(*p)
	{
        addch(*p);
        p++;
        refresh();
        napms(20);
	}
	do{
		keyInput=getch();
	}while( keyInput != 'c');  // bloque l'ecran tant qu'on a pas appuyer sur la touche c
	clear();
}


char *choices[] = {
                        "Play",
                        "Help",
                        "Exit"
                  };

int n_choices= sizeof(choices) / sizeof(char *);

int menu(char *game[])
{       
	WINDOW *menu_win;
    int highlight = 1;
    int choice = 0;
    int c;
	int row,col;
	
    getmaxyx(stdscr,row,col);
	noecho();
    menu_win = newwin(HEIGHT_MENU, WIDTH_MENU, (row/2) - HEIGHT_MENU/2, (col/2) - WIDTH_MENU/2);
	start_color();
    init_pair(1,COLOR_WHITE,COLOR_MAGENTA);
	wrefresh(menu_win);
	wbkgd(menu_win, COLOR_PAIR(1));
	keypad(menu_win, TRUE);
    mvprintw(0, 0, "Use arrow keys to go up and down, Press enter to select a choice");
	refresh();
    print_menu(menu_win, highlight);
    while(1)
    {      
		c = wgetch(menu_win);
        switch(c)
        {       
			case KEY_UP:
            	if(highlight == 1)
                    highlight = n_choices;
                else
                    --highlight;
                break;
            case KEY_DOWN:
                if(highlight == n_choices)
                    highlight = 1;
                else
                    ++highlight;
                break;
            case 10:  // 10 = touche entrée
                choice = highlight;
                break;
            default:
                mvprintw(24, 0, "Charcter pressed is = %3d Hopefully it can be printed as '%c'",c,c);
		        refresh();
                break;
        }
        print_menu(menu_win, highlight);
		if(choice == 1){  // case play
			delwin(menu_win);
			refresh();
			play(game);
			break;
		}/* call function depeding on the choice of the user */
		if (choice == 3) break; // case exit
		if (choice == 2){ // case help
			delwin(menu_win);
			refresh();
			help(game);
			break;
		}
        }
        
        return 0;
}

void print_menu(WINDOW *menu_win, int highlight)
{
    int x, y, i;
    x = 2;
    y = 2;
    box(menu_win, 0, 0);
    for(i = 0; i < n_choices; ++i)
    {       
		if(highlight == i + 1) /* High light the present choice */
        {
			wattron(menu_win, A_REVERSE);
            mvwprintw(menu_win, y, x, "%s", choices[i]);
            wattroff(menu_win, A_REVERSE);
        }
        else
            mvwprintw(menu_win, y, x, "%s", choices[i]);
        ++y;
    }
    wrefresh(menu_win);
}

void help(char *game[])
{
	clear();
	start_color();
    init_pair(1,COLOR_WHITE,COLOR_MAGENTA);
    bkgd(COLOR_PAIR(1));
	refresh();

	WINDOW *help_win;
	int row, col,x,y;
	x = 2;
	y = 2;
	getmaxyx(stdscr,row,col);
	help_win = newwin(HEIGHT, WIDTH, (row/2) - HEIGHT/2, (col/2) - WIDTH/2);
	wbkgd(help_win,COLOR_PAIR(1));
	wrefresh(help_win);
	keypad(help_win, TRUE);
	box(help_win,0,0);
	mvwprintw(help_win, y, x, "programme de Yann et Hugo");
    mvwprintw(help_win, y+1, x, "Nous avons programmer par nos soins le snake et le boulet ");
    mvwprintw(help_win, y+2, x, "Nous vous proposons donc dans ce petit programme une base de donnée avec plusieurs jeux ");
    mvwprintw(help_win, y+3, x, "Pour plus d'informations veuillez vous référer au README dans notre github ;)" );
	wrefresh(help_win);
	mvprintw(0, 0, "Please enter c to continue");
	refresh();
	int keyInput;
	do{
        keyInput=getch();
    }while( keyInput != 'c');
	delwin(help_win);
    clear();
	menu(game);
}

void play(char *game[])
{	
    ITEM **my_items;				
	MENU *my_menu;
    WINDOW *my_menu_win;
    int n_games, i;
    int choice = 0;
    int highlight = 1;
	
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_CYAN, COLOR_BLACK);

	/* Create items */
    n_games = 6;
    my_items = (ITEM **)calloc(n_games, sizeof(ITEM *));
	
    for(i = 0; i < n_games; ++i){
		my_items[i] = new_item(game[i+2],"");
	}     
	/* Create menu */
	my_menu = new_menu((ITEM **)my_items);
	
	/* Create the window to be associated with the menu */
    int row,col;
    getmaxyx(stdscr,row,col);
    my_menu_win = newwin(10, 40, (row/2)- 5,(col/2)- 20);
    keypad(my_menu_win, TRUE);
     
	/* Set main window and sub window */
    set_menu_win(my_menu, my_menu_win);
    set_menu_sub(my_menu, derwin(my_menu_win, 6, 38, 3, 1));
	set_menu_format(my_menu, 5, 1);
			
	/* Set menu mark to the string " * " */
    set_menu_mark(my_menu, " * ");

	/* Print a border around the main window and print a title */
    box(my_menu_win, 0, 0);
	print_in_middle(my_menu_win, 1, 0, 40, "Games by ESME", COLOR_PAIR(1));
	mvwaddch(my_menu_win, 2, 0, ACS_LTEE);
	mvwhline(my_menu_win, 2, 1, ACS_HLINE, 38);
	mvwaddch(my_menu_win, 2, 39, ACS_RTEE);
        
	/* Post the menu */
	post_menu(my_menu);
	wrefresh(my_menu_win);
	refresh();

	int c;
	while(1)
	{      
        c = wgetch(my_menu_win); 
        switch(c)
	    {	
			case KEY_DOWN:
                if(highlight != n_games)
					++highlight;				
				menu_driver(my_menu, REQ_DOWN_ITEM);
				break;
			case KEY_UP:
                if(highlight != 1)
					--highlight;
				menu_driver(my_menu, REQ_UP_ITEM);
				break;
			case KEY_NPAGE:
				menu_driver(my_menu, REQ_SCR_DPAGE);
				break;
			case KEY_PPAGE:
				menu_driver(my_menu, REQ_SCR_UPAGE);
				break;
            case 10:
                choice = highlight;
                break;
			default:
				mvprintw(24, 0, "Charcter pressed is = %3d Hopefully it can be printed as '%c'", c, c);
				refresh();
				break;
		}
    	wrefresh(my_menu_win);
    	if(choice != 0)	/* User did a choice come out of the infinite loop */
			break;
	}	
	clrtoeol();
	delwin(my_menu_win);
	refresh();
           
	/* Unpost and free all the memory taken up */
    unpost_menu(my_menu);
    free_menu(my_menu);
    for(i = 0; i < n_games; ++i)
    free_item(my_items[i]);
	endwin();
	// on appelle le code avec execv
	const char* str1 = "./games/";
    const char* str2 = game[choice + 1];
    const char* str3 = "/exe.sh";

    char buffer1[100];
    strcat(strcpy(buffer1, str1), str2);
	char buffer2[100];
	strcat(strcpy(buffer2, buffer1), str3);
	execv(buffer2,NULL);	
        
}

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color)
{	int length, x, y;
	float temp;

	if(win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if(startx != 0)
		x = startx;
	if(starty != 0)
		y = starty;
	if(width == 0)
		width = 80;

	length = strlen(string);
	temp = (width - length)/ 2;
	x = startx + (int)temp;
	wattron(win, color);
	mvwprintw(win, y, x, "%s", string);
	wattroff(win, color);
	refresh();
}

