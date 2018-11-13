#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/msg.h> 
 
#include "header.h"
 struct mesg_buffer { 
    long mesg_type; 
    char mesg_text[100]; 
} message; 
int main(int argc, char *argv[]) { 	
        key_t key; 
    	int msgid; 
	int timeid = atoi(argv[1]);
	key = ftok("progfile", 65);
	msgid = msgget(key, 0666 | IPC_CREAT); 
    	message.mesg_type = 1; 
	pid_t pid = getpid();
	unsigned int *seconds = 0, *nanoseconds = 0;
      
	attachToSharedMemory(&seconds, &nanoseconds, timeid);
	sprintf(message.mesg_text, "USER %d MESSAGE \n", getpid());
	initializeUser(&seconds, &nanoseconds);
	//printf("after init\n");
	//printf("USER: %d\n", pid);
	//printf("USER: seconds are: %u nano are: %u\n", *seconds, *nanoseconds);
 	//printf("After printf\n");
	while(1){
		msgsnd(msgid, &message, sizeof(message), 0);	
		//printf("2\n");
		sleep(1);
		msgrcv(msgid, &message, sizeof(message), 1, 0);
	}
	shmdt(seconds);
     	//shmdt(msg);
	return 0;
}
