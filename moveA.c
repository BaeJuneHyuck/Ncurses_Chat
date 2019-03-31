#include <ncurses.h>
#include <unistd.h>

int main()
{
  int row = 10, col = 10;
  initscr();	// initilize the window
  noecho();	// don't echo any key presses
  curs_set(FALSE);	// don't display a cursor
  keypad(stdscr,TRUE);	//standard screen
  clear();
  mvprintw(row,col,"AoA");
  refresh();
  while(1){
    int input = getch();
    clear();
    switch(input){
      case KEY_UP:    --row,col;break;
      case KEY_DOWN:  ++row,col;break;
      case KEY_LEFT:  row,--col;break;
      case KEY_RIGHT: row,++col;break;
    }
    mvprintw(row,col,"AoA");
    if(input == 'q')break;
    refresh();
  }
  
  endwin();
  return 0;
}


