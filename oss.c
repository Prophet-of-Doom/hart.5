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
int checkArrPosition(PCB *pcbPtr, int *position){
        int i = 0;
        for(i = 0; i < 18; i++){
                if(pcbPtr[i].isSet == 0){
			//printf("Position %d is free\n", i);
                        *position = i;
                        return 1;
                }
        }
	return 0;
}
void initializeResourceArray(resourceDesc *resourcePtr){
	int i;
	for(i = 0; i < 20; i++){
		resourcePtr[i].resources = rand()%(10)+1;
		resourcePtr[i].max = resourcePtr[i].resources;
		//printf("Resource %d was assigned with %d resources\n", i, resourcePtr[i].resources);
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
		//random = rand()%limit;
		//pcbPtr[position].resourceRequirements[i] = random; //Sets the requirement for every resource
	}
}
void printProcessLimits(resourceDesc *resourcePtr, PCB *pcbPtr, FILE *logFile){
	fprintf(logFile,"\nPROCESS MAX TABLE (The most each process gets)\n");
	int i, j;
	fprintf(logFile,"\t");
	for(i = 0; i < 20; i++){
		fprintf(logFile,"R%d\t", i);
	}
	for (i = 0; i < 18; i++){
		if(pcbPtr[i].isSet == 1){
			fprintf(logFile,"\nP%d\t", i);
			for(j = 0; j < 20; j++){
				fprintf(logFile,"%d\t", pcbPtr[i].resourceLimits[j]);
			}
		}
	}
}
void printProcessRequirement(resourceDesc *resourcePtr, PCB *pcbPtr, FILE *logFile){
	fprintf(logFile,"\nPROCESS REQUIREMENT TABLE (what each process needs)\n");
	int i, j;
	fprintf(logFile,"\t");
	for(i = 0; i < 20; i++){
		fprintf(logFile,"R%d\t", i);
	}
	for (i = 0; i < 18; i++){
		if(pcbPtr[i].isSet == 1){
			fprintf(logFile,"\nP%d\t", i);
			for(j = 0; j < 20; j++){
				fprintf(logFile,"%d\t", pcbPtr[i].resourceRequirements[j]);
			}
		}
	}
}
void printProcessAllocation(resourceDesc *resourcePtr, PCB *pcbPtr, FILE *logFile){
	fprintf(logFile,"\nPROCESS ALLOCATION TABLE (what each process currently has)\n");
	int i, j;
	fprintf(logFile,"\t");
	for(i = 0; i < 20; i++){
		fprintf(logFile,"R%d\t", i);
	}
	for (i = 0; i < 18; i++){
		if(pcbPtr[i].isSet == 1){
			fprintf(logFile,"\nP%d\t", i);
			for(j = 0; j < 20; j++){
				fprintf(logFile,"%d\t", pcbPtr[i].resourcesAllocated[j]);
			}
		}
	}
}

void printResources(resourceDesc *resourcePtr, PCB *pcbPtr, FILE *logFile){
	fprintf(logFile, "\nCURRENT RESOURCE TABLE (what each resource currently has)\n");
	int i;
	fprintf(logFile,"\t");
	for(i = 0; i < 20; i++){
		fprintf(logFile,"R%d\t", i);
	}
	fprintf(logFile,"\nMAX:\t");
	for (i = 0; i < 20; i++){
		fprintf(logFile,"%d\t", resourcePtr[i].max);
	}	
	fprintf(logFile,"\nCRNT:\t");
	for (i = 0; i < 20; i++){
		fprintf(logFile,"%d\t", resourcePtr[i].resources);
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
	int verbose = 1;
	srand(time(NULL));
	alrm = 0;
	
	char* filename = malloc(sizeof(char));
	filename = "log.txt";
        FILE *logFile = fopen(filename, "a");

	key_t msgKey = ftok(".", 'G'), timeKey = 0, pcbKey = 0, resourceKey = 0; 
	int msgid = msgget(msgKey, 0666 | IPC_CREAT), timeid = 0, pcbid = 0 ,lines = 0,resourceid = 0, position = 0, requestsGranted=1, deadlockAvoidance = 0, releases = 0;
	unsigned int *seconds = 0, *nanoseconds = 0, forkTimeSeconds = 0, forkTimeNanoseconds = 0;
	PCB *pcbPtr = NULL;	
	resourceDesc *resourcePtr = NULL;
	char sharedTimeMem[10], sharedPCBMem[10], sharedPositionMem[10], sharedResourceMem[10];
	//pid_t pid = 0;
	
        createSharedMemKeys(&resourceKey, &timeKey, &pcbKey);
        createSharedMemory(&timeid, &pcbid, &resourceid, timeKey, pcbKey, resourceKey);
	attachToSharedMemory(&seconds, &nanoseconds, &pcbPtr ,&resourcePtr, timeid, pcbid, resourceid);
	//printf("OSS timeid %d pcbid %d resourceid %d\n", timeid, pcbid, resourceid);

	initializeResourceArray(resourcePtr);
	//printResources(resourcePtr, pcbPtr);
	
	int forked = 0, forkTimeSet = 0, i = 0;
	char childRequestType[20], childRequestResource[20], childIdentifier[20], ch;
	signal(SIGALRM, timerKiller);
        alarm(2);
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
			//printf("OSS: Fork time set for %d.%d\n", forkTimeSeconds, forkTimeNanoseconds);
		}		
		*nanoseconds += 50000;
		if (*nanoseconds >= 1000000000){ //billion
                	*seconds += 1;
                       	*nanoseconds = 0;
                }
		if((*seconds == forkTimeSeconds && *nanoseconds >= forkTimeNanoseconds) || *seconds > forkTimeSeconds){
			if(checkArrPosition(pcbPtr, &position) == 1){ //also gotta check to make sure theres no more than 18	
				//printf("PCB: %p\n", &resourcePtr[0]);		
				
				pcbPtr[position].position = position;
				initializePCBArrays(pcbPtr, position, resourcePtr);
				//printf("OSS: local position is %d pcb position is %d\n", position, pcbPtr[position].position);
				createArgs(sharedTimeMem, sharedPCBMem, sharedPositionMem, sharedResourceMem, timeid, pcbid ,resourceid, position);
				forkChild(sharedTimeMem, sharedPCBMem, sharedPositionMem, sharedResourceMem, seconds, nanoseconds, &position);
				//printf("OSS: Created a user at %u.%u\n", *seconds, *nanoseconds);
				//sleep(2);
				pcbPtr[position].complete = 0;
				forked++;
				//printf("FORKING! %d\n", forked);
				//for(i = 0; i < 20; i ++){ // initial resource allocation.
				//	if(pcbPtr[i].isSet == 1){
				//		printf("IN HERE ASSHOLE\n");
						resourceAllocation(pcbPtr, resourcePtr, position);
							//add to blocked queue;
							
							//break;
						
				//	}
				//}
				//printProcessLimits(resourcePtr, pcbPtr);
				//printProcessRequirement(resourcePtr, pcbPtr);
				//printProcessAllocation(resourcePtr, pcbPtr);
				//printResources(resourcePtr, pcbPtr);
				//printf("\nOSS: Before MSG RCV\n");
				//msgrcv(msgid, &message, sizeof(message), 1, 0);
				//printf("OSS: Message received is %s\n", message.mesg_text);
			}
			forkTimeSet = 0;
			
		}
		//loop that goes through every user and checks to see if theres a request on that channel
		
		for(i = 0; i < 18; i++){
			if(pcbPtr[i].isSet == 1 && pcbPtr[i].waitingToBlock == 0){
				if(msgrcv(msgid, &message, sizeof(message), pcbPtr[i].pid, IPC_NOWAIT) > 0){				
					strcpy(childIdentifier, strtok(message.mesg_text," "));		
					if(atoi(childIdentifier) == 27){	
						strcpy(childRequestResource, strtok(NULL, " "));
						strcpy(childRequestType, strtok(NULL, " "));		
						if(atoi(childRequestType) == 1){
							//request resource;
							//maybe have function return a value for whether it should be blocked
							//change requirement based on msg
							//printf("11111111111111111111111111111111111111111111111111111111111111111111111111111\n");			
							if((pcbPtr[i].resourcesAllocated[atoi(childRequestResource)] + 1) <= pcbPtr[i].resourceLimits[atoi(childRequestResource)]){
							//printf("22222222222222222222222222222222222222222222222222222222222222222222222222222\n");
								if(verbose)
									fprintf(logFile, "OSS: Process %d wants %d at %u.%u.\n", i, atoi(childRequestResource), *seconds, *nanoseconds); 
								pcbPtr[i].resourceRequirements[atoi(childRequestResource)] += 1;
								if(resourceAllocation(pcbPtr, resourcePtr, i) == 0){
									//add to blocked queue;
									fprintf(logFile, "OSS: P%d is being blocked at %u.%u.\n", i, *seconds, *nanoseconds);
								} else {
									//printProcessLimits(resourcePtr, pcbPtr);
									//printProcessRequirement(resourcePtr, pcbPtr);
									//printProcessAllocation(resourcePtr, pcbPtr);
									//printResources(resourcePtr, pcbPtr);
									requestsGranted++;
								}
							}
						} else if(atoi(childRequestType) == 0){
							if(verbose)
								fprintf(logFile, "OSS: Process %d releases %d at %u.%u.\n", i, atoi(childRequestResource), *seconds, *nanoseconds); 
							//release resource;
							releases++;
							if((pcbPtr[i].resourcesAllocated[atoi(childRequestResource)] - 1) >= 0){
								pcbPtr[i].resourceRequirements[atoi(childRequestResource)] += 1;
								resourceRelease(pcbPtr, resourcePtr, i);
								//printProcessLimits(resourcePtr, pcbPtr);
								//printProcessRequirement(resourcePtr, pcbPtr);
								//printProcessAllocation(resourcePtr, pcbPtr);
								//printResources(resourcePtr, pcbPtr);
							}
						} else {
							fprintf(logFile, "User process is complete at %u:%u and made %d requests with %d fulfilled, made %d releases, and was blocked %d times.\n", *seconds, *nanoseconds, pcbPtr[i].requestsMade, pcbPtr[i].requestsGranted, pcbPtr[i].releasesMade, pcbPtr[i].timesBlocked);
							clearPcb(pcbPtr, resourcePtr, i);	
						}
						message.mesg_type = pcbPtr[i].pid;
						sprintf(message.mesg_text,"wake up shithead");
						//printf("OSS sent wake up message.\n");						
						msgsnd(msgid, &message, sizeof(message), pcbPtr[i].pid);
						pcbPtr[i].post = 1;
					}
				}
			}
		}
		for(i = 0; i < 18; i++){
			
			//if theres memory available in resourceptr.resources 
				//then go through all blocked processes and see if they need it
					//if it does then  give it and unblock it
				//then go through all unblocked processes and see if they need anything
			if(pcbPtr[i].waitingToBlock == 1 && pcbPtr[i].isSet == 1){
				unblockProcess(pcbPtr, resourcePtr, i, &deadlockAvoidance);
				if(pcbPtr[i].waitingToBlock == 0)
					fprintf(logFile,"OSS: use %d is being unblocked at %u:%u.\n", i, *seconds, *nanoseconds); 
			}

			
		}
		if(requestsGranted %20 == 0){
			printProcessLimits(resourcePtr, pcbPtr, logFile);
			printProcessRequirement(resourcePtr, pcbPtr, logFile);
			printProcessAllocation(resourcePtr, pcbPtr, logFile);
			printResources(resourcePtr, pcbPtr, logFile);
		}
		/*for(i = 0; i < 18; i++){
			if(pcbPtr[i].isSet == 1){
				printf("sent message\n");
				message.mesg_type = pcbPtr[i].pid;
				sprintf(message.mesg_text,"wake up shithead");
				msgsnd(msgid, &message, sizeof(message), pcbPtr[i].pid);
			}
		}*/
		
		/*Maybe I'd have a function check for terminating. Like have the user set a 
		terminating variable right before it dies. Oh also what if I had a for loop that was the size of 
		all running processes and checked for a message rcv. BUT how would it know that the same child is 
		sending more than 1 message?*/
		while(!feof(logFile)){
			ch = fgetc(logFile);
			if(ch == '\n'){
				lines++;
			}
		}
		if(lines >= 100000){
			fclose(logFile);
		}
	}while((*seconds < 20) && alrm == 0 && forked < 101);
	//printf("OSS: OUT OF LOOP1\n");	
	for(i = 0; i < 18; i++){
		if(pcbPtr[i].isSet == 1){
			pcbPtr[i].post = 1;		
			pcbPtr[i].complete = 1;
		}	
	}
	for(i = 0; i < 18; i++){
		if(pcbPtr[i].isSet == 1){
			//printf("waiting for message receive from %d\n.", i);
			msgrcv(msgid, &message, sizeof(message), pcbPtr[i].pid, 0);
			fprintf(logFile, "User made %d requests with %d fulfilled, made %d releases, and was blocked %d times.\n", pcbPtr[i].requestsMade, pcbPtr[i].requestsGranted, pcbPtr[i].releasesMade, pcbPtr[i].timesBlocked);
		}
	}
	//printf("OSS: OUT OF LOOP2\n");	
	
 	fclose(logFile);
    
	shmdt(seconds);
	shmdt(pcbPtr);
	shmdt(resourcePtr);
	msgctl(msgid, IPC_RMID, NULL);
        shmctl(msgid, IPC_RMID, NULL);
	shmctl(timeid, IPC_RMID, NULL);
	shmctl(pcbid, IPC_RMID, NULL);
	shmctl(resourceid, IPC_RMID, NULL);
	kill(0, SIGTERM);
	return 0;
}
