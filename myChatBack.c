#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERNAME 32

struct message{
  char *str;
  int length;
  char from[MAX_USERNAME];
};

void printWindow(char* user,WINDOW *input,WINDOW *output, WINDOW* userlist){

    wborder(output,'|','|','-','-','*','*','*','*');
    wborder(userlist,'|','|','-','-','*','*','*','*');
    wborder(input,'|','|','-','-','*','*','*','*');
    
    wattron(userlist,A_BOLD);
    wattron(userlist,COLOR_PAIR(1));
    mvwprintw(userlist, 1, 1, user);
    wattroff(userlist,A_BOLD);
    wattroff(userlist,COLOR_PAIR(1));
    mvwprintw(input, 1, 1, "input:");
    
    wrefresh(output);
    wrefresh(userlist);
    wrefresh(input);
}

int main()
{
  int output_x = 50;
  int output_y = 15;
  int input_x = output_x;
  int input_y = 6;
  int userlist_x = 30;
  int userlist_y = output_y + input_y;
  char *username;
  initscr();
  noecho();
  cbreak();
  //curs_set(false);
  username = getenv("USER");

  WINDOW *output = newwin(output_y, output_x, 0, 0);
  WINDOW *userlist = newwin(userlist_y, userlist_x, 0, output_x);
  WINDOW *input = newwin(input_y, input_x, output_y, 0);  
  
  scrollok(output,true);
  scrollok(input,true);
  scrollok(userlist,true);

  start_color();
  init_pair(1,COLOR_RED,COLOR_WHITE);

  printWindow(username, input,output,userlist);
  char inputline[300] ="";
  char outputline[600] = " ";
  char c;
  while(c = wgetch(input)){ 
    if(c == 27) break;
    else if(c == 10){
      size_t outlen = strlen(outputline);
      size_t inlen = strlen(inputline);
      char *newline = (char*)malloc(outlen+inlen+3);
      strcpy(newline,outputline);
      newline[outlen]='\n';
      //strcpy(newline,"\n");
      //strcpy(newline,inputline);
      strcat(newline,inputline);
      newline[outlen+inlen+2]='\0';
      strcpy(outputline, newline);
      wclear(output);
      wprintw(output,newline);
      memset(inputline,' ',300);
      inputline[0] = '\0';
      wclear(input);
      free(newline);
    }else if(c == 127){ 
      size_t len = strlen(inputline);
      if (len <= 0) continue;
      char *newline = (char *)malloc(len-1);
      strcpy(newline,inputline);
      newline[len-1] = '\0';
      strcpy(inputline,newline);
      wclear(input);
      free(newline);
    }else{
      size_t len = strlen(inputline);
      char *newline = (char*)malloc(len+1);
      strcpy(newline,inputline);
      newline[len] = c;
      newline[len+1] = '\0';
      strcpy(inputline,newline);
      free(newline);
    }
    printWindow(username, input,output,userlist);
    wprintw(input,inputline);
  }

  //sleep(1);

  delwin(output);
  delwin(input);
  delwin(userlist);

  endwin();
  return 0;
}
