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
        key_t msgKey = ftok(".", 'G');
	int msgid = msgget(msgKey, 0666 | IPC_CREAT);
	int timeid = atoi(argv[1]);
	int pcbid = atoi(argv[2]);
	int position = atoi(argv[3]);
	int resourceid = atoi(argv[4]);
	int event = 0, eventResource = 0, requestsMade = 0, releasesMade = 0;	
	pid_t pid = getpid();
	unsigned int *seconds = 0, *nanoseconds = 0, eventTimeSeconds = 0, eventTimeNanoseconds = 0;
        resourceDesc *resourcePtr = NULL;
	PCB *pcbPtr = NULL;
	
	attachToSharedMemory(&seconds, &nanoseconds, &pcbPtr, &resourcePtr, timeid, pcbid, resourceid);
	initializeUser(&seconds, &nanoseconds, pcbPtr, position);
	
	pcbPtr[position].position = position;
	//so the user is given a maximum amount it can ask from each resource, then in user it randomly
	//decides how much of the resource it needs. 
	
	setRandomEventTime(seconds, nanoseconds, &eventTimeSeconds, &eventTimeNanoseconds);
	//initializeUser(&seconds, &nanoseconds, pcbPtr, position);
	message.mesg_type = pid;
	pcbPtr[position].isSet = 1;
	while(pcbPtr[position].complete == 0){
		//msgrcv(msgid, &message, sizeof(message), pid, 0);
		if(pcbPtr[position].waitingToBlock == 0){
	
			//printf("Process P%d is unblocked and waiting for event at %d:%d at %d:%d\n", position, *seconds, *nanoseconds, eventTimeSeconds, eventTimeNanoseconds);
			if((*seconds == eventTimeSeconds && *nanoseconds >= eventTimeNanoseconds) || *seconds > eventTimeSeconds){						
				event = rand()%99;
				if(event >= 0 && event < 10){
					//printf("USER: P%d Event DEATH block status %d.\n", position, pcbPtr[position].waitingToBlock);
					//kill & release all
					sprintf(message.mesg_text,"%d %d %d user dead\n", 27, 0, 2);
					msgsnd(msgid, &message, sizeof(message), 0);
					while(pcbPtr[position].post == 0){
						//nothing
						//printf(".");
					}
					msgrcv(msgid, &message, sizeof(message), pid, 0);
					pcbPtr[position].post = 0;
					pcbPtr[position].complete = 1;
				} else if (event >= 10 && event < 65){
					//request random
					//printf("USER: P%d Event REQUEST block status %d.\n", position, pcbPtr[position].waitingToBlock);
					eventResource = rand()%20;
					sprintf(message.mesg_text,"%d %d %d user wants x\n", 27, eventResource, 1);
					msgsnd(msgid, &message, sizeof(message), 0);
					while(pcbPtr[position].post == 0){
						//nothing
						//printf(".");
					}
					msgrcv(msgid, &message, sizeof(message), pid, 0);
					pcbPtr[position].requestsMade++;
					pcbPtr[position].post = 0;
					//may block child somehow as it waits for confirmation.
				} else if (event >= 65){
					//release random
					//printf("USER: P%d Event RELEASE block status %d.\n", position, pcbPtr[position].waitingToBlock);
					eventResource = rand()%20;
					sprintf(message.mesg_text,"%d %d %d user releases x\n", 27, eventResource, 0);
					msgsnd(msgid, &message, sizeof(message), 0);
					while(pcbPtr[position].post == 0){
						//nothing
						//printf(".");
					}
					msgrcv(msgid, &message, sizeof(message), pid, 0);
					pcbPtr[position].releasesMade++;
					pcbPtr[position].post = 0;
				}
				setRandomEventTime(seconds, nanoseconds, &eventTimeSeconds, &eventTimeNanoseconds);
			}
		}
	}

	
	sprintf(message.mesg_text,"user %d position being passed is %d and the pcb is %d\n", pid, position, pcbPtr[position].position);
	//printf("USER: Before MSG SEND\n");
	msgsnd(msgid, &message, sizeof(message), pid);
	//printf("USER: AFTER msgsnd %d\n", position);
	//printf("USER: position %d pid %d\n", position, pid);
	//printf("USER: seconds are: %u nano are: %u\n", *seconds, *nanoseconds);
	shmdt(seconds);
     	shmdt(pcbPtr);
	shmdt(resourcePtr);
	msgctl(msgid, IPC_RMID, NULL);
	return 0;
}
