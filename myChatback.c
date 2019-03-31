#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERNAME 32
#define MAX_OUTLINE 15
#define MAX_BUFFER 1024

typedef struct message{
  char *str;
  int length;
  char from[MAX_USERNAME];
  struct message* link;
}message;

typedef struct messageQueue{
  message *front;
  message *rear;
  int size;
}messageQueue;

void initQueue (messageQueue q){
  q.front = q.rear = NULL;
  q.size = 0;
}

void enqueue(messageQueue q, char* _from, char* _str){
  if(q.size >= MAX_OUTLINE){
    message* temp;
    temp = q.front;
    q.front = q.front->link;
    free(temp);
  }else{
    q.size++;
    message* temp;
    temp = (message *)malloc(sizeof(message));
    strcpy(temp->from,_from);
    strcpy(temp->str,_str);
    if(q.rear == NULL) q.front = q.rear = temp;
    else{
      q.rear->link = temp;
      q.rear = temp;
    }
  }
}

void printWindow(char* user,WINDOW* input,WINDOW* output, WINDOW* userlist){

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

void printMessages(WINDOW* output, messageQueue mQueue){
  int i = 0;
  char *totalStr = (char *)malloc(MAX_BUFFER);
  message* temp = mQueue.front;
    // queue message -> print  
  while(temp){
    char *curStr = temp->from;
    strcpy(curStr,":");
    strcpy(curStr,temp->str);
    size_t totallen = strlen(totalStr);
    size_t curlen = strlen(curStr);
    char *newline = (char*)malloc(totallen+curlen+3);
    strcpy(totalStr,curStr);
    totalStr[totallen]='\n';
    
    strcat(totalStr,curStr);
    totalStr[totallen+curlen+2]='\0';
  }
  
  wclear(output);
  wprintw(output,totalStr);
  free(totalStr);
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
  messageQueue mqueue;
  
  initQueue(mqueue);
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
  char inputline[500] ="";
  char outputline[600] = " ";
  char c;
  while(c = wgetch(input)){ 
    if(c == 27) break;  // esc
    else if(c == 10){ // enter
     /* 
      size_t outlen = strlen(outputline);
      size_t inlen = strlen(inputline);
      char *newline = (char*)malloc(outlen+inlen+3);
      strcpy(newline,outputline);
      newline[outlen]='\n';
      
      strcat(newline,inputline);
      newline[outlen+inlen+2]='\0';
      strcpy(outputline, newline);
     */
      //wclear(output);
      //wprintw(output,newline);
      enqueue(mqueue,username,inputline);
      printMessages(output,mqueue);
      //clear input line
      memset(inputline,' ',300);
      inputline[0] = '\0';
      wclear(input);
    }else if(c == 127){ // backspace
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
