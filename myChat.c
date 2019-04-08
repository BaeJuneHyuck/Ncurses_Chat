#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h> // getenv, malloc, free
#include <string.h> //strcpy, strncpy, strcat
#include <errno.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>

#define MAX_USER 10
#define MAX_USERNAME 32
#define MAX_BUFFER 2048
#define MAX_MESSAGE 20
#define MAX_MESSAGE_LENGTH 185
#define SHARED_MEMORY 5120

const int output_x = 50;
const int output_y = 18;
const int input_x =  50;
const int input_y = 6;
const int userlist_x = 30;
const int userlist_y = 24;

union semun{
  int val;
  struct semid_ds *buf;
  unsigned short int *array;
}semopts;

void p(int semid){
  struct sembuf pbuf;
  pbuf.sem_num = 0;
  pbuf.sem_op = -1;
  pbuf.sem_flg  = SEM_UNDO;
  semop(semid, &pbuf, 1);
}

void v(int semid){
  struct sembuf vbuf;
  vbuf.sem_num = 0;
  vbuf.sem_op = 1;
  vbuf.sem_flg  = SEM_UNDO;
  semop(semid, &vbuf, 1);
}

typedef struct userTable{
  char name[MAX_USER][MAX_USERNAME];
  bool using[MAX_USER];
}userTable;

typedef struct message{
  char str[MAX_MESSAGE_LENGTH];
  char from[MAX_USERNAME];
  int length;
}message;

typedef struct messageQueue{ // circular queue
  int usercount; // number of user in chat room
  int userkey;   // unique number for each user, never decrease
  bool roomActivated;
  userTable usertable;
  message list[MAX_MESSAGE];
  int size;
  int rear;
  int front;
}messageQueue;

void initQueue (messageQueue* q){
  q->front = q->rear = -1;
  q->size = MAX_MESSAGE;
  q->usercount = 0;
  q->roomActivated = false;
  for(int i = 0 ;i<MAX_USER;i++){
    q->usertable.using[i] = false;
    strcpy(q->usertable.name[i], "");
  }
}

bool adduser(int semid, messageQueue* q, char*name){
  if(q->usercount > MAX_USER) return false;
  //get in critical section
 
//  p(semid);
  q->usercount++;
  q->roomActivated = true;
  q->usertable.using[q->usercount] = true;
  strcpy(q->usertable.name[q->usercount], name);
//  v(semid);
 
  return true;
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
    int totallen = strlen(totalStr);
    int curlen = strlen(curStr);
    
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

void printWindow(WINDOW* input,WINDOW* output){ 
  mvwprintw(input, 0, 0, "input:");
  wrefresh(output); 
  wrefresh(input);
}

void printUser(WINDOW* userlist, userTable* table){
  wattron(userlist,A_BOLD);
  wattron(userlist,COLOR_PAIR(1));
 
  // get users from table
  // if (using) it means there are user here 
  char *totalStr = (char *)malloc(MAX_BUFFER);
  strcpy(totalStr,"\0"); // init str '\0'
  for(int i = 0 ; i < MAX_USER ; i++){
    if(table->using[i]){
      strcat(totalStr," ");
      strcat(totalStr, table->name[i]);
      strcat(totalStr, "\n"); 
    }
  }
  strcat(totalStr,"\0");

  wclear(userlist);
  mvwprintw(userlist, 0, 0, totalStr); // finally print total string
  free(totalStr); // don't forget free
  
  wattroff(userlist,A_BOLD);
  wattroff(userlist,COLOR_PAIR(1));
  wrefresh(userlist);
}

int main(int argc, char*argv[]){
  int myKey;       
  int currentUser; // for checking userlist refresh
  size_t shmid;
  int semid;
  pid_t server_pid;
  messageQueue* mqueue;
  char *username;

  // get name from argument or use account name
  if(argc>1 && strlen(argv[1])<MAX_USERNAME){
    username = (char *)malloc(sizeof(char) * strlen(argv[1]));
    strcpy(username, argv[1]);
  }else{
    username = getenv("USER");
  }

  // create or get shared memory and semaphore
  semid = semget(950426, 1,  IPC_CREAT | 0777);
  shmid = shmget(201424465, SHARED_MEMORY, IPC_EXCL| IPC_CREAT | 0777);
  if(shmid == -1){ // shared memory exists! 
    shmid = shmget(201424465, SHARED_MEMORY, 0777);
    mqueue =(messageQueue*) shmat(shmid,NULL,0);
  }
  else{ // if not, make shared memory, init, make server process
    mqueue = shmat(shmid,NULL,0);
    initQueue(mqueue);
    printf("fork ready\n");
    server_pid = fork();
    if(server_pid == 0){
      //child == server
      //check user, if 0 delete shared memory, and semaphore
      while(!mqueue->roomActivated){
        printf("waiting for room activated!\n");
      }
      while(1){
        if(mqueue->usercount == 0){
	  break;
        }
      }
      semctl(semid, 0, IPC_RMID, 0);
      shmctl(shmid, IPC_RMID, NULL);
      exit(0);
    }
  }  
  
  semopts.val = 1;

  // if room is full, exit
  if(!adduser(semid, mqueue,username)){
    printf("the chat room is full!\n");
    exit(1);
  }

  //add user to userlist
  myKey = mqueue->usercount;
  mqueue->usertable.using[myKey] == true;
  strcpy(mqueue->usertable.name[myKey], username);

  //init ncurses
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

  printUser(userlist,&(mqueue->usertable));
  printWindow(input,output);
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
        // critical section : using semaphore
        p(semid);
	mqueue->usertable.using[myKey]=false;
	mqueue->usercount--;
        v(semid);
        shmdt(mqueue);
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
    // check userlist, update if needed
    if(currentUser != mqueue->usercount){
      printUser(userlist,&(mqueue->usertable));
    }
    printWindow(input,output);
    wprintw(input,inputline);    
  }
  delwin(output);
  delwin(input);
  delwin(userlist);

  endwin();
  return 0;
}
