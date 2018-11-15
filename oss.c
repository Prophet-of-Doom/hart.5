#include "header.h"
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
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
	int i, random, outerlimit, limit;  
	//ok so my question is, does this limit take into account the resource amount?
	//so like instead of that 4 would i put the max of the resourceDesc?
	//but that doesnt make sense because it doesnt take into account the other resources
	//Sets the resource limit for user
	outerlimit = rand()%10+1;
	for(i = 0; i < 20; i++){	
		//limit = rand()%10+1;//resourcePtr[i].max+1;
		if(outerlimit > resourcePtr[i].max){
			limit = resourcePtr[i].max;
		} else {
			limit = outerlimit;
		}
		pcbPtr[position].resourceLimits[i] = limit;
		random = rand()%limit+1;
		pcbPtr[position].resourceRequirements[i] = random; //Sets the requirement for every resource
	}
}
void printProcessLimits(resourceDesc *resourcePtr, PCB *pcbPtr){
	printf("PROCESS MAX TABLE (The most each process gets)\n");
	int i, j;
	printf("\t");
	for(i = 0; i < 20; i++){
		printf("R%d\t", i);
	}
	for (i = 0; i < 18; i++){
		printf("\nP%d\t", i);
		for(j = 0; j < 20; j++){
			printf("%d\t", pcbPtr[i].resourceLimits[j]);
		}
	}
}
void printProcessRequirement(resourceDesc *resourcePtr, PCB *pcbPtr){
	printf("PROCESS REQUIREMENT TABLE (what each process needs)\n");
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
void printProcessAllocation(resourceDesc *resourcePtr, PCB *pcbPtr){
	printf("\nPROCESS ALLOCATION TABLE (what each process currently has)\n");
	int i, j;
	printf("\t");
	for(i = 0; i < 20; i++){
		printf("R%d\t", i);
	}
	for (i = 0; i < 18; i++){
		printf("\nP%d\t", i);
		for(j = 0; j < 20; j++){
			printf("%d\t", pcbPtr[i].resourcesAllocated[j]);
		}
	}
}

void printResources(resourceDesc *resourcePtr, PCB *pcbPtr){
	printf("\nCURRENT RESOURCE TABLE (what each resource currently has)\n");
	int i;
	printf("\t");
	for(i = 0; i < 20; i++){
		printf("R%d\t", i);
	}
	printf("\nMAX:\t");
	for (i = 0; i < 20; i++){
		printf("%d\t", resourcePtr[i].max);
	}	
	printf("\nCRNT:\t");
	for (i = 0; i < 20; i++){
		printf("%d\t", resourcePtr[i].resources);
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
	printResources(resourcePtr, pcbPtr);
	
	int forked = 0, forkTimeSet = 0, i = 0;
	char childRequestType[10], childRequestResource[10];
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
		if(forked == 0){
			printf("PCB: %p\n", &resourcePtr[0]);		
			forkTimeSet = 0;
			checkArrPosition(pcbPtr, &position);//maybe have this return a value if its full or not and encapsulate the rest of this in it		
			pcbPtr[position].isSet = 1;
			pcbPtr[position].position = position;
			initializePCBArrays(pcbPtr, position, resourcePtr);
			printf("OSS: local position is %d pcb position is %d\n", position, pcbPtr[position].position);
			createArgs(sharedTimeMem, sharedPCBMem, sharedPositionMem, sharedResourceMem, timeid, pcbid ,resourceid, position);
			forkChild(sharedTimeMem, sharedPCBMem, sharedPositionMem, sharedResourceMem, seconds, nanoseconds, &position);
			printf("OSS: Created a user at %u.%u\n", *seconds, *nanoseconds);
			//sleep(2);
			forked++;
			//for(i = 0; i < 20; i ++){ // initial resource allocation.
			//	if(pcbPtr[i].isSet == 1){
			//		printf("IN HERE ASSHOLE\n");
					if(resourceAllocation(pcbPtr, resourcePtr, position) == 0){
						//add to blocked queue;
						enqueueBlockedProcess(pcbPtr);
						//break;
					}
			//	}
			//}
			printProcessLimits(resourcePtr, pcbPtr);
			printProcessRequirement(resourcePtr, pcbPtr);
			printProcessAllocation(resourcePtr, pcbPtr);
			printResources(resourcePtr, pcbPtr);
			//printf("\nOSS: Before MSG RCV\n");
			//msgrcv(msgid, &message, sizeof(message), 1, 0);
			//printf("OSS: Message received is %s\n", message.mesg_text);
		}}
		//loop that goes through every user and checks to see if theres a request on that channel
		for(i = 0; i < 18; i++){
			if(pcbPtr[i].isSet == 1){
				if(msgrcv(msgid, &message, sizeof(message), pcbPtr[i].pid, IPC_NOWAIT) > 0){
					printf("OSS: Message received is %s\n", message.mesg_text);
					strcpy(childRequestResource, strtok(message.mesg_text, " "));
					strcpy(childRequestType, strtok(NULL, " "));
					if(atoi(childRequestType) == 1){
						//request resource;
						//maybe have function return a value for whether it should be blocked
						//change requirement based on msg
						pcbPtr[i].resourceRequirements[atoi(childRequestResource)] += 1;
						if(resourceAllocation(pcbPtr, resourcePtr, i) == 0){
							//add to blocked queue;
							printf("OSS: Adding P%d to blocked queue\n", i);
							enqueueBlockedProcess(pcbPtr);
						} else {
							printProcessLimits(resourcePtr, pcbPtr);
							printProcessRequirement(resourcePtr, pcbPtr);
							printProcessAllocation(resourcePtr, pcbPtr);
							printResources(resourcePtr, pcbPtr);
						}
					} else {
						//release resource;
					}
					sprintf(message.mesg_text,"wake up shithead");
					msgsnd(msgid, &message, sizeof(message), pid);
				}
			}
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
