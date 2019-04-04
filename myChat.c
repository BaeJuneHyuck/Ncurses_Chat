#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h> // getenv, malloc, free
#include <string.h> //strcpy, strncpy, strcat
#include <pthread.h>
#include <sys/shm.h>

#define MAX_USERNAME 32
#define MAX_BUFFER 2048
#define MAX_MESSAGE_LENGTH 185
#define MAX_OUTLINE 15

const int output_x = 50;
const int output_y = 18;
const int input_x =  50;
const int input_y = 6;
const int userlist_x = 30;
const int userlist_y = 24;

typedef struct message{
  //char str[MAX_MESSAGE_LENGTH];
  //char from[MAX_USERNAME];
  char *str;
  char *from;
  int length;
  struct message* link;
}message;

typedef struct messageQueue{
  message *front;
  message *rear;
  int size;
  int usercount;
}messageQueue;

void initQueue (messageQueue* q){
  q->front = q->rear = NULL;
  q->size = 0;
  q->usercount = 1;
}

// add a message to message queue
// using dynamic memory allocation
void enqueue(messageQueue* q, char* _from, char* _str, int length){
  q->size++;
  message* temp = (message *)malloc(sizeof(message));
  temp->from = (char *)malloc(sizeof(char) * MAX_USERNAME);
  temp->str = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
  temp->link = NULL;
  temp->length = length;
  strcpy(temp->from,_from);
  strncpy(temp->str,_str,length);

  // if queue is empty or not
  if(q->rear == NULL) q->front = q->rear = temp;
  else{
    q->rear->link = temp;
    q->rear = temp;
  }
}

// delete message from queue
// free used memory
void dequeue(messageQueue* q){
  q->size--;
  message* temp = q->front;
  q->front = q->front->link;

  //free(temp->from);
  //free(temp->str);
  free(temp);
}

// print all Messages in messagequeue
// get message instance with temp->link
// merge all characters into one 'totalStr' with strcat
void printMessages(WINDOW* output, messageQueue* q){
  char *totalStr = (char *)malloc(MAX_BUFFER);
  strcpy(totalStr,"\0"); // init str '\0'
  message* temp = q->front;
  
  int t = 0;

  while(temp){
    char *curStr = (char *)malloc(MAX_MESSAGE_LENGTH + MAX_USERNAME + 6);
    strcpy(curStr," "); // set string empty
    
    // fill current string with username, message
    strcat(curStr, temp->from);
    strcat(curStr," : ");
    strncat(curStr,temp->str,temp->length);
    strcat(curStr,"\0");
    size_t totallen = strlen(totalStr);
    size_t curlen = strlen(curStr);
    
    // concatenate current string(temp) to total string
    strcat(totalStr,curStr);
    totalStr[totallen]='\n';
    totalStr[totallen+curlen+2]='\0';
    
    // select next message, free temp char*
    temp = temp->link;
    free(curStr);
  }
  
  wclear(output);
  mvwprintw(output, 0, 0, totalStr); // finally print total string
  free(totalStr); // don't forget free
}

void printWindow(char* user,WINDOW* input,WINDOW* output, WINDOW* userlist){
    wattron(userlist,A_BOLD);
    wattron(userlist,COLOR_PAIR(1));
    mvwprintw(userlist, 0,0, user);
    wattroff(userlist,A_BOLD);
    wattroff(userlist,COLOR_PAIR(1));
    
    mvwprintw(input, 0, 0, "input:");
    wrefresh(output); 
    wrefresh(userlist);
    wrefresh(input);
}


int main(){
  // mqueue has all messages from user
 
  //messageQueue* mqueue = (messageQueue *)malloc(sizeof(messageQueue));
  
  int shmid;
  pid_t server_pid;
  messageQueue* mqueue;
  
  shmid = shmget(201424465, sizeof(messageQueue), IPC_EXCL| IPC_CREAT | 0660);
  if(shmid == -1){ // shm already exists! 
    shmid = shmget(201424465, sizeof(messageQueue), 0660);
    mqueue =(messageQueue*) shmat(shmid,NULL,0);
    mqueue->usercount++;
  }
  else{
    mqueue = shmat(shmid,NULL,0);
    initQueue(mqueue);
    
    server_pid == fork();
    if(server_pid == 0){
      //child == server
      while(mqueue->usercount != 0);
      shmctl(shmid, IPC_RMID, NULL);
      exit(0);
    }else{
      // parent, =client
    }
  }  

  char *username;
  username = getenv("USER");

  initscr();
  noecho();
  cbreak();
  keypad(stdscr, true);
 
  // make window
  WINDOW *output_border   = newwin(output_y, output_x, 0, 0);
  WINDOW *userlist_border = newwin(userlist_y, userlist_x, 0, output_x);
  WINDOW *input_border    = newwin(input_y, input_x, output_y, 0);   
  WINDOW *output          = newwin(output_y-2, output_x-2, 1, 1);
  WINDOW *userlist        = newwin(userlist_y-2, userlist_x-2, 1, output_x+1);
  WINDOW *input           = newwin(input_y-2, input_x-2, output_y+1, 1);  
  
  scrollok(output,true);
  scrollok(input,true);
  scrollok(userlist,true);

  start_color();
  init_pair(1,COLOR_BLACK,COLOR_WHITE);

  //make border(box)
  box(input_border,0,0);
  box(userlist_border,0,0);
  box(output_border,0,0);
  wrefresh(output_border);
  wrefresh(userlist_border);
  wrefresh(input_border);
  
  printWindow(username, input,output,userlist);

  char inputline[MAX_MESSAGE_LENGTH] ="";
  int  index = 0;
  char outputline[MAX_BUFFER] = "";
  char c;

  while(c = wgetch(input)){ 
    //if(c == 27) continue;  // ignore special function keys like esc,f1,f2, etc
    if(c == 10){ 
      // enter, make a message from inputline and enqueue
      // if queue is full dequeue the oldest one
      if (index <= 0) continue;
      if(strcmp(inputline, "./quit") == 0){
	mqueue->usercount--;
	break;
      }
      if(mqueue->size > MAX_OUTLINE) dequeue(mqueue);
      inputline[index++] = '\0';
      enqueue(mqueue,username,inputline,index);
      printMessages(output,mqueue); //update output screen
      
      //clear input line
      memset(inputline,'\0',MAX_MESSAGE_LENGTH);
      index = 0;
      wclear(input);
    }else if(c == 127){
      // backspace, remove the last chracter from inputline
      if (index <= 0) continue;
      inputline[index-1] = '\0';
      index--;
      wclear(input);
    }else{
      // else, add a chracter to inputline
      if (index >= MAX_MESSAGE_LENGTH)continue;
      inputline[index] = c;
      inputline[index+1] = '\0';
      index++;
    } 
    printWindow(username, input,output,userlist);
    wprintw(input,inputline);
  }
  //free(mqueue);

  //shmctl(shmid, IPC_RMID, NULL);
  
  delwin(output);
  delwin(input);
  delwin(userlist);

  endwin();
  return 0;
}
