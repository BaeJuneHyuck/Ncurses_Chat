#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h> // getenv, malloc, free
#include <string.h> //strcpy, strncpy, strcat
#include <pthread.h>
#include <sys/shm.h>

#define MAX_USERNAME 32
#define MAX_BUFFER 2048
#define MAX_MESSAGE_LENGTH 185
#define MAX_MESSAGE 20
#define SHARED_MEMORY 4668

// one messageQueue = 2* message + 2* int = 2*message + 8
// one messag       = char*[185 + 32] + int + pointer to next
//                  = 217 + 8 + 8  =233
// shm need one message queue + 20 outline(message instance)
// (8 + 20*233 = 4668)??

const int output_x = 50;
const int output_y = 18;
const int input_x =  50;
const int input_y = 6;
const int userlist_x = 30;
const int userlist_y = 24;

typedef struct message{
  char str[MAX_MESSAGE_LENGTH];
  char from[MAX_USERNAME];
  int length;
}message;

typedef struct messageQueue{ // circular queue
  int usercount;
  message list[MAX_MESSAGE];
  int size;
  int rear;
  int front;
}messageQueue;

void initQueue (messageQueue* q){
  q->front = q->rear = -1;
  q->size = MAX_MESSAGE;
  q->usercount = 1;
}

// add a message to message queue
void enqueue(messageQueue* q, char* _from, char* _str, int length){
  if((q->front == 0 && q->rear == q->size - 1) || 
		  (q->rear == (q->front-1)%(q->size-1))){
    // queue is full, overwrite firt message
    q->front ++;
    q->rear ++;
  }else if(q->front == -1){ // first input
    q->front = q->rear = 0;
  }else if(q->rear == q->size -1 && q->front != 0){
    q->rear = 0;
  }
  else{
    q->rear++;
  }

  //message* temp = (message *)malloc(sizeof(message));
  //message* temp = q->list[q->rear];
  //temp->from = (char *)malloc(sizeof(char) * MAX_USERNAME);
  //temp->str = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
  message *temp = &q->list[q->rear];
  temp->length = length;
  strcpy(temp->from,_from);
  strncpy(temp->str,_str,length);
}

void makeMessage(char* totalStr, message* msg){
    char *curStr = (char *)malloc(MAX_MESSAGE_LENGTH + MAX_USERNAME + 6);
    strcpy(curStr," "); // set string empty
    
    // fill current string with username, message
    strcat(curStr, msg->from);
    strcat(curStr," : ");
    strncat(curStr,msg->str,msg->length);
    strcat(curStr,"\0");
    size_t totallen = strlen(totalStr);
    size_t curlen = strlen(curStr);
    
    // concatenate current string(temp) to total string
    strcat(totalStr,curStr);
    totalStr[totallen]='\n';
    totalStr[totallen+curlen+2]='\0';
    
    // select next message, free temp char*
    free(curStr);
}

// print all Messages in messagequeue
void printMessages(WINDOW* output, messageQueue* q){
  char *totalStr = (char *)malloc(MAX_BUFFER);
  strcpy(totalStr,"\0"); // init str '\0'
  
  int front = q->front;
  int rear  = q->rear;
  int size  = q-> size;
  if(rear >= front){
    for(int i = front ; i<= rear;i++){
      makeMessage(totalStr,&(q->list[i]));
    }
  }else{
    for(int i = front ; i < size ;i++){ 
      makeMessage(totalStr,&(q->list[i]));
    }
    for(int i = 0 ; i<= rear;i++){
      makeMessage(totalStr,&(q->list[i]));
    }
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


int main(int argc, char*argv[]){
  // mqueue has all messages from user
  
  size_t shmid;
  pid_t server_pid;
  messageQueue* mqueue;
  
  char *username;
  if(argc>1 && strlen(argv[1])<MAX_USERNAME){
    username = (char *)malloc(sizeof(char) * strlen(argv[1]));
    strcpy(username, argv[1]);
  }else{
    username = getenv("USER");
  }
  shmid = shmget(201424465, SHARED_MEMORY, IPC_EXCL| IPC_CREAT | 0777);
  if(shmid == -1){ // shm already exists! 
    shmid = shmget(201424465, SHARED_MEMORY, 0777);
    mqueue =(messageQueue*) shmat(shmid,NULL,0);
    mqueue->usercount++;
  }
  else{
    mqueue = shmat(shmid,NULL,0);
    initQueue(mqueue);
    
    server_pid = fork();
    if(server_pid == 0){
      //child == serve
      while(mqueue->usercount != 0);
      shmctl(shmid, IPC_RMID, NULL);
      exit(0);
    }
  }  

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
  //shmdt(shmid);
  delwin(output);
  delwin(input);
  delwin(userlist);

  endwin();
  return 0;
}
