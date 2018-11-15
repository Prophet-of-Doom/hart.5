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
void setRandomEventTime(unsigned int *seconds, unsigned int *nanoseconds, unsigned int *eventTimeSeconds, unsigned int *eventTimeNanoseconds){
	unsigned int random = rand()%1000000000;
	*eventTimeNanoseconds = 0;
	*eventTimeSeconds = 0;	
	if((random + *nanoseconds) >=1000000000){
		*eventTimeSeconds += 1;
		*eventTimeNanoseconds = (random + *nanoseconds) - 1000000000;
	} else {
		*eventTimeNanoseconds = random + *nanoseconds;
	}
	*eventTimeSeconds = *seconds + rand()%2;
}
int main(int argc, char *argv[]) { 	
	printf("What fuck1111111111111111111111111111111111111111111111111111111111111\n");
        key_t msgKey = ftok(".", 'G');
	int msgid = msgget(msgKey, 0666 | IPC_CREAT);
	int timeid = atoi(argv[1]);
	int pcbid = atoi(argv[2]);
	int position = atoi(argv[3]);
	int resourceid = atoi(argv[4]);

	int complete = 0, event = 0, eventResource = 0;	
	pid_t pid = getpid();
	unsigned int *seconds = 0, *nanoseconds = 0, eventTimeSeconds = 0, eventTimeNanoseconds = 0;
        resourceDesc *resourcePtr = NULL;
	PCB *pcbPtr = NULL;
	printf("What fuck2222222222222222222222222222222222222222222222222222222\n");
	attachToSharedMemory(&seconds, &nanoseconds, &pcbPtr, &resourcePtr, timeid, pcbid, resourceid);
	printf("User pcb: %p\n", &resourcePtr[position]);
	printf("What fuck33333333333333333333333333333333333333333333333333333333333333\n");
	initializeUser(&seconds, &nanoseconds, pcbPtr, position);
	printf("What fuck44444444444444444444444444444444444444444444444444444444\n");
	
	//int test = pcbPtr[position].isSet;
	printf("WHAT FUCL $#@$@#$#@$#@$@#$@#$@#$@#$\n");
	pcbPtr[position].position = position;
	//so the user is given a maximum amount it can ask from each resource, then in user it randomly
	//decides how much of the resource it needs. 
	
	setRandomEventTime(seconds, nanoseconds, &eventTimeSeconds, &eventTimeNanoseconds);
	//initializeUser(&seconds, &nanoseconds, pcbPtr, position);
	message.mesg_type = pid;

	while(complete == 0){
		
		if(pcbPtr[position].waitingToBlock == 0){
			//printf("What fuck!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			if((*seconds == eventTimeSeconds && *nanoseconds >= eventTimeNanoseconds) || *seconds > eventTimeSeconds){						
				printf("USER: Event happening.\n");
				event = 30;//rand()%99;
				if(event >= 0 && event < 20){
					//kill & release all
					complete = 1;
				} else if (event >= 20 && event < 60){
					//request random
					eventResource = rand()%20;
					sprintf(message.mesg_text,"%d %d user wants x\n", eventResource, 1);
					msgsnd(msgid, &message, sizeof(message), pid);
					msgrcv(msgid, &message, sizeof(message), pid, 0);
					//may block child somehow as it waits for confirmation.
				} else if (event >= 60){
					//release random
					eventResource = rand()%20;
					sprintf(message.mesg_text,"%d %d user releases x\n", eventResource, 0);
					msgsnd(msgid, &message, sizeof(message), pid);
				}
				setRandomEventTime(seconds, nanoseconds, &eventTimeSeconds, &eventTimeNanoseconds);
			}
		}
	}

	
	//sprintf(message.mesg_text,"user %d position being passed is %d and the pcb is %d\n", pid, position, pcbPtr[position].position);
	//printf("USER: Before MSG SEND\n");
	//msgsnd(msgid, &message, sizeof(message), 0);
	//printf("USER: AFTER msgsnd\n");
	printf("USER: %d\n", pid);
	printf("USER: seconds are: %u nano are: %u\n", *seconds, *nanoseconds);
	shmdt(seconds);
     	shmdt(pcbPtr);
	return 0;
}
