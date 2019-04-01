#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h> // getenv, malloc, free
#include <string.h>

#define MAX_USERNAME 32
#define MAX_BUFFER 15360
#define MAX_MESSAGE_LENGTH 1024
#define MAX_OUTLINE 15

typedef struct message{
  char *str;
  char *from;
  struct message* link;
}message;

typedef struct messageQueue{
  message *front;
  message *rear;
  int size;
}messageQueue;

void initQueue (messageQueue* q){
  q->front = q->rear = NULL;
  q->size = 0;
}

void enqueue(messageQueue* q, char* _from, char* _str){
  q->size++;
  message* temp = (message *)malloc(sizeof(message));
  temp->from = (char *)malloc(sizeof(char) * MAX_USERNAME);
  temp->str = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
  temp->link = NULL;
  strcpy(temp->from,_from);
  strcpy(temp->str,_str);

  if(q->rear == NULL) q->front = q->rear = temp;
  else{
    q->rear->link = temp;
    q->rear = temp;
  }
}

void dequeue(messageQueue* q){
  q->size--;
  message* temp = q->front;
  q->front = q->front->link;
  /*
  free(temp->from);
  free(temp->str);
  free(temp);
*/
 }

void printMessages(WINDOW* output, messageQueue* q){
  char *totalStr = (char *)malloc(MAX_BUFFER);
  message* temp = q->front;
  
  // queue message -> print  
  while(temp){
    char *curStr = (char *)malloc(MAX_MESSAGE_LENGTH + MAX_USERNAME + 6);
    strcpy(curStr, "  ");
    strcat(curStr, temp->from);
    strcat(curStr," : ");
    strcat(curStr,temp->str);
    strcat(curStr,"\0");
    size_t totallen = strlen(totalStr);
    size_t curlen = strlen(curStr);
    strcat(totalStr,curStr);
    totalStr[totallen]='\n';
    
    totalStr[totallen+curlen+2]='\0';
    temp = temp->link;
    free(curStr);
  }
  
  wclear(output);
  wprintw(output,totalStr);
  free(totalStr);
}

void printWindow(char* user,WINDOW* input,WINDOW* output, WINDOW* userlist){

    wborder(output,0,0,0,0,0,0,0,0);
    wborder(userlist,0,0,0,0,0,0,0,0);
    wborder(input,0,0,0,0,0,0,0,0);
    
    wattron(userlist,A_BOLD);
    wattron(userlist,COLOR_PAIR(1));
    mvwprintw(userlist, 1, 1, user);
    wattroff(userlist,A_BOLD);
    wattroff(userlist,COLOR_PAIR(1));
    
    int i;
    //mvwprintw(output, 0, 0, "*");
    //mvwprintw(input, 1, 0, "*");
    //for (i = 1; i < 50; i++){
    //  mvwprintw(input, 1, i, "-");
    //  mvwprintw(output, 0, i, "-");
    // }
    //mvwprintw(input, 1, 49, "*");
    mvwprintw(input, 1, 1, "input:");
    
    wrefresh(output);
    wrefresh(userlist);
    wrefresh(input);
}
    
int main(){
  int output_x = 50;
  int output_y = 18;
  int input_x = output_x;
  int input_y = 6;
  int userlist_x = 30;
  int userlist_y = output_y + input_y;
  char *username;
  messageQueue* mqueue = (messageQueue *)malloc(sizeof(messageQueue));
  
  initQueue(mqueue);
  initscr();
  noecho();
  cbreak();
  //curs_set(false);
  username = getenv("USER");

  WINDOW *output = newwin(output_y-1, output_x, 0, 0);
  WINDOW *userlist = newwin(userlist_y, userlist_x, 0, output_x);
  WINDOW *input = newwin(input_y, input_x, output_y, 0);  
  
  scrollok(output,true);
  scrollok(input,true);
  scrollok(userlist,true);

  start_color();
  init_pair(1,COLOR_BLACK,COLOR_WHITE);

  printWindow(username, input,output,userlist);
  char inputline[MAX_MESSAGE_LENGTH] ="";
  char outputline[MAX_BUFFER] = " ";
  char c;
  while(c = wgetch(input)){ 
    if(c == 27) break;  // esc
    else if(c == 10){ // enter
      size_t len = strlen(inputline);
      if (len <= 0) continue;
      if(mqueue->size > MAX_OUTLINE){
        dequeue(mqueue);
        wclear(output);
      }
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
  free(mqueue);

  delwin(output);
  delwin(input);
  delwin(userlist);

  endwin();
  return 0;
}
