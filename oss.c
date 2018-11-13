#include "header.h"
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <time.h>
struct mesg_buffer { 
    long mesg_type; 
    char mesg_text[100]; 
} message; 
int alrm;

void timerKiller(int sign_no){
        alrm = 1;
}
void checkArrPosition(PCB *pcbPtr, int *position){
        int i = 0;
        for(i = 0; i < 18; i++){
                if(pcbPtr[i].isSet == 0){
			printf("yeah %d\n", i);
                        *position = i;
                        break;
                }
        }
}
void initializeResourceArray(resourceDesc *resourcePtr){
	int i;
	for(i = 0; i < 20; i++){
		resourcePtr[i].resources = rand()%(10)+1;
		resourcePtr[i].max = resourcePtr[i].resources;
		printf("Resource %d was assigned with %d resources\n", i, resourcePtr[i].resources);
	}
}

void initializePCBArrays(PCB *pcbPtr, int position, resourceDesc *resourcePtr){
	int i, random, limit;  
	//ok so my question is, does this limit take into account the resource amount?
	//so like instead of that 4 would i put the max of the resourceDesc?
	//but that doesnt make sense because it doesnt take into account the other resources
	//Sets the resource limit for user
	for(i = 0; i < 20; i++){
		
		limit = rand()%10+1;//resourcePtr[i].max+1;
		if(limit > resourcePtr[i].max){
			limit = resourcePtr[i].max;
		}
		pcbPtr[position].resourceLimits[i] = limit;
		random = rand()%limit+1;
		pcbPtr[position].resourceRequirements[i] = random; //Sets the requirement for every resource
	}
}
void printLog(resourceDesc *resourcePtr, PCB *pcbPtr){
	int i, j;
	printf("\t");
	for(i = 0; i < 20; i++){
		printf("R%d\t", i);
	}
	for (i = 0; i < 18; i++){
		printf("\nP%d\t", i);
		for(j = 0; j < 20; j++){
			printf("%d\t", pcbPtr[i].resourceRequirements[j]);
		}
	}
}
void setRandomForktime(unsigned int *seconds, unsigned int *nanoseconds, unsigned int *forkTimeSeconds, unsigned int *forkTimeNanoseconds){
	unsigned int random = rand()%1000000000;
	*forkTimeNanoseconds = 0;
	*forkTimeSeconds = 0;	
	if((random + *nanoseconds) >=1000000000){
		*forkTimeSeconds += 1;
		*forkTimeNanoseconds = (random + *nanoseconds) - 1000000000;
	} else {
		*forkTimeNanoseconds = random + *nanoseconds;
	}
	*forkTimeSeconds = *seconds + rand()%2;
}
		  
int main(int argc, char *argv[]){
	srand(time(NULL));
	alrm = 0;
	
	char* filename = malloc(sizeof(char));
	filename = "log.txt";
        FILE *logFile = fopen(filename, "a");

	key_t msgKey = ftok(".", 'G'), timeKey = 0, pcbKey = 0, resourceKey = 0; 
	int msgid = msgget(msgKey, 0666 | IPC_CREAT), timeid = 0, pcbid = 0 ,resourceid = 0, position = 0;
	unsigned int *seconds = 0, *nanoseconds = 0, forkTimeSeconds = 0, forkTimeNanoseconds = 0;
	PCB *pcbPtr = NULL;	
	resourceDesc *resourcePtr = NULL;
	char sharedTimeMem[10], sharedPCBMem[10], sharedPositionMem[10], sharedResourceMem[10];
	//pid_t pid = 0;
	
        createSharedMemKeys(&resourceKey, &timeKey, &pcbKey);
        createSharedMemory(&timeid, &pcbid, &resourceid, timeKey, pcbKey, resourceKey);
	attachToSharedMemory(&seconds, &nanoseconds, &pcbPtr ,&resourcePtr, timeid, pcbid, resourceid);

      	initBlockedQueue();
	initializeResourceArray(resourcePtr);	
	//initHPQueue();
 	
	int forked = 0, forkTimeSet = 0;
	//signal(SIGALRM, timerKiller);
        //alarm(2);
	do{		
		//sessentially i think it would be easier to fork 18 children first and in the process
		//of doing so you set up the PCB requirements so every time you fork a child within the PCB you 
		//set its shit
		/*Actually heres what its gonna do, theres a main loop that creates the random times to create children*/	
		//printf("OSS: At top current time %d.%d\n", *seconds, *nanoseconds);
		/*Ok so now what I need to do is keep track of what processes are alive and not blocked
		I can probably do it in here. Children also terminate on their own so I don't need to waitpid
		I just check for a message constantly*/
		if(forkTimeSet == 0){		
			setRandomForktime(seconds, nanoseconds, &forkTimeSeconds, &forkTimeNanoseconds);
			forkTimeSet = 1;
			printf("OSS: Fork time set for %d.%d\n", forkTimeSeconds, forkTimeNanoseconds);
		}		
		*nanoseconds += 5000;
		if (*nanoseconds >= 1000000000){ //billion
                	*seconds += 1;
                       	*nanoseconds = 0;
                }
		if((*seconds == forkTimeSeconds && *nanoseconds >= forkTimeNanoseconds) || *seconds > forkTimeSeconds){
			forkTimeSet = 0;
			checkArrPosition(pcbPtr, &position);		
			pcbPtr[position].isSet = 1;
			pcbPtr[position].position = position;
			initializePCBArrays(pcbPtr, position, resourcePtr);
			printf("OSS: local position is %d pcb position is %d\n", position, pcbPtr[position].position);
			createArgs(sharedTimeMem, sharedPCBMem, sharedPositionMem, sharedResourceMem, timeid, pcbid ,resourceid, position);
			forkChild(sharedTimeMem, sharedPCBMem, sharedPositionMem, sharedResourceMem, seconds, nanoseconds, &position);
			printf("OSS: Created a user at %u.%u\n", *seconds, *nanoseconds);
			//sleep(2);
			forked++;
			printLog(resourcePtr, pcbPtr);
			printf("OSS: Before MSG RCV\n");
			msgrcv(msgid, &message, sizeof(message), 1, 0);
			printf("OSS: Message received is %s\n", message.mesg_text);
		}
		/*Maybe I'd have a function check for terminating. Like have the user set a 
		terminating variable right before it dies. Oh also what if I had a for loop that was the size of 
		all running processes and checked for a message rcv. BUT how would it know that the same child is 
		sending more than 1 message?*/
	}while((*seconds < 10) && alrm == 0 && forked < 18);

	printf("OSS: OUT OF LOOP\n");	
	
 	fclose(logFile);
    
	shmdt(seconds);
	shmdt(pcbPtr);
	msgctl(msgid, IPC_RMID, NULL);
        shmctl(msgid, IPC_RMID, NULL);
	shmctl(timeid, IPC_RMID, NULL);
	shmctl(pcbid, IPC_RMID, NULL);
	kill(0, SIGTERM);
	return 0;
}
