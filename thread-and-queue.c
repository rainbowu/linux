#include <pthread.h>    
#include <stdio.h>    
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <error.h>
#include <signal.h>
#include <stddef.h>
#include "ncurses.h"
#include <sys/time.h> 
#include <cstdlib> 


#define MAX_TEXT 64
#define usleep_long 1000000
#define usleep_middle 200000
#define usleep_low 100000
int msgid=0;
int shutdown = 0;
int sendtime = 50000;
int rectime = 300000;
bool dynamic_buttom = true;
timeval begin;

typedef struct {
  int my_msg_type;
  char msg_text[MAX_TEXT];
} my_msg_st;


// Press ctrl + C
void ctrl_c(int signo){

  //notify pthread close.
  shutdown = 1;

  //delete message queue.
  msgctl( msgid, IPC_RMID,NULL) ;
}



// dynamic check per 2 seconds
void times_up(int signo){
  
  float percent;
  
  msqid_ds info ;  
  //msgget( msgid , 0666 | IPC_CREAT) ;

  msgctl( msgid , IPC_STAT, &info ) ;
  percent = (float)info.msg_cbytes / (float)info.msg_qbytes ;

  
  // > 0.60
  if (percent > 0.60 && dynamic_buttom == true){
    sendtime = usleep_long;
    rectime = usleep_low;
    dynamic_buttom = false; 
  }
  // <0.35
  if(percent < 0.35 && dynamic_buttom == false){
    sendtime = usleep_low;
    rectime = usleep_long;
    dynamic_buttom = true; 
  }

}





// controller
void *control(void *argu) {

    msqid_ds info ;
    int flag ;
    float percent;
    char tmp = '%'; // printw seems ignoring % during " ".
    initscr();

    //msgget( msgid , 0666 | IPC_CREAT) ;

  while (1 && !shutdown ) {    
	
    msgctl( msgid , IPC_STAT, &info ) ;
    percent = (float)info.msg_cbytes / (float)info.msg_qbytes ;

    move(5,30);
    printw("message information");
    move(6,18);
    printw("==============================================");
    move(7,20);
    printw("system total byte of message    = %9d" , info.msg_qbytes);
    move(8,20); 
    printw("current total number of message = %9d" , info.msg_qnum);
    move(9,20);
    printw("current total byte of message   = %9d" , info.msg_cbytes);
    move(10,18);
    printw("==============================================");
    move(11,18);
    printw("Queue:");
    move(11,25);
    for(float i=0;i<25;i++){
      if(i < percent*25)
	printw("|");
      else
        printw("_");	
    }
    move(12,18);
    printw("Queue Level: %.0f", percent*100  );
    move(12,34); 
    printw("%c",tmp);	
    move(14,30);
    
    move(15,18);
    //  current time
    timeval current;
    gettimeofday(&current, (struct timezone*) 0);

    /* calculate difference */
    int second = (current.tv_sec - begin.tv_sec) ;
    int minute = second/60;
    int hour = minute/60;

    printw("Elapse Time: %d hour %d minute %d second" , hour%24 , minute%60 , second % 60 );
    refresh(); 
  }    
  return NULL;    
}    


// sender
void *producer(void *argu) {    
 
  my_msg_st some_data;

 
  char buffer[MAX_TEXT];
  // create msg queue
  msgid = msgget( IPC_PRIVATE , 0666 | IPC_CREAT) ; 

  while ( 1 && !shutdown) {    
    strcpy(some_data.msg_text, "hello \n") ;    	
    some_data.my_msg_type = 1;
    
    
     msgsnd(msgid, (void *)&some_data, MAX_TEXT, 0);
    

     usleep(sendtime);     
  }    
  return NULL;    
}    



// receiver
void *comsumer(void *argu) {     
  int running = 1;
  my_msg_st some_data;
  int msg_to_receive = 0;

  //msgget( msgid , 0666 | IPC_CREAT);
 
  while (running && !shutdown) {

    
    msgrcv(msgid, (void *)&some_data, BUFSIZ, msg_to_receive, 0);

    //printf("recieve msg : %s", some_data.msg_text);

    usleep(rectime);    

	
  }    

  return NULL;    
}    




int main(int argc, char *argv[]) {     	

//set up exit.
  struct sigaction exit;
  exit.sa_handler = ctrl_c;
  sigemptyset(&exit.sa_mask);
  exit.sa_flags=0;
  sigaction(SIGINT, &exit, NULL);


//set up timer
  
  struct sigaction timer;
  timer.sa_handler = times_up;
  sigemptyset(&timer.sa_mask);
  timer.sa_flags=0;
  sigaction(SIGALRM, &timer, NULL);

// t = init:5 sec + interval:2 sec
  struct itimerval t;
  t.it_interval.tv_usec = 0;
  t.it_interval.tv_sec = 1;
  t.it_value.tv_usec = 0;
  t.it_value.tv_sec = 1;
  

  if ( argc >=2 && !strncmp( argv[1] , "d" , 1)  ){
  setitimer( ITIMER_REAL, &t, NULL);
  }			

// program running time
    
  gettimeofday(&begin, 0);
 
  pthread_t controler_thread, sensor_thread, device_thread;
  pthread_attr_t  controler_attr, sensor_attr,device_attr;
  struct sched_param controler_sched_value, sensor_sched_value,  device_sched_value;
   
  pthread_attr_init(&sensor_attr);
  pthread_attr_init(&device_attr);
  pthread_attr_init(&controler_attr);
  
  pthread_attr_setschedpolicy(&controler_attr, SCHED_RR);
  pthread_attr_setschedpolicy(&sensor_attr, SCHED_RR);
  pthread_attr_setschedpolicy(&device_attr, SCHED_RR);
  
  controler_sched_value.sched_priority = 6;
  sensor_sched_value.sched_priority = 6;
  device_sched_value.sched_priority = 6;

  pthread_create(&controler_thread,  &controler_attr, &producer, NULL);    	
  pthread_create(&sensor_thread, &sensor_attr, &comsumer, NULL);   
  pthread_create(&device_thread, &device_attr,  &control , NULL);   

  pthread_join(controler_thread , NULL);
  pthread_join(sensor_thread , NULL);
  pthread_join(device_thread , NULL);

  system("clear");
  endwin();  

  return 0;    

}

