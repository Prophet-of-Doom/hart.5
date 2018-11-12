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
        key_t msgKey = ftok(".", 'G');
	int msgid = msgget(msgKey, 0666 | IPC_CREAT);
	int timeid = atoi(argv[1]);
	int pcbid = atoi(argv[2]);
	int position = atoi(argv[3]);
	int resourceid = atoi(argv[4]);
	pid_t pid = getpid();
	unsigned int *seconds = 0, *nanoseconds = 0;
        resourceDesc *resourcePtr = NULL;
	PCB *pcbPtr = NULL;

	attachToSharedMemory(&seconds, &nanoseconds, &pcbPtr, &resourcePtr, timeid, pcbid, resourceid);
	pcbPtr[position].position = position;
	pcbPtr[position].isSet = 1;
	//so the user is given a maximum amount it can ask from each resource, then in user it randomly
	//decides how much of the resource it needs. 
	initializeUser(&seconds, &nanoseconds, pcbPtr);
	printf("USER: after init\n");
	message.mesg_type = 1;
	sprintf(message.mesg_text,"user %d position being passed is %d and the pcb is %d\n", pid, position, pcbPtr[position].position);
	printf("USER: Before MSG SEND\n");
	msgsnd(msgid, &message, sizeof(message), 0);
	printf("USER: AFTER msgsnd\n");
	printf("USER: %d\n", pid);
	printf("USER: seconds are: %u nano are: %u\n", *seconds, *nanoseconds);
	shmdt(seconds);
     	shmdt(pcbPtr);
	return 0;
}
