#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <sys/msg.h>
#include <string.h>
#define STRUCT_ARRAY_SIZE ((sizeof(pcbArray)/sizeof(pcbArray[0])) * 18)
#define SEM_SIZE sizeof(int)
#define QUEUE_SIZE 18
//for the queue
#ifndef FALSE
#define FALSE (0)
#endif
//For true
#ifndef TRUE
#define TRUE (!FALSE)
#endif

typedef struct resourceDesc{
	int resources;
	int max;
}resourceDesc;

typedef struct PCB{
	int waitingToBlock;
	int position;
	int isSet;
	int resourceLimits[20];
	int resourceRequirements[20];
	int resourcesAllocated[20];
	pid_t pid;
}PCB;

typedef struct QNodeType{
	PCB *qptr;
	struct QNodeType *next;
	//struct QNodeType *currentNode;
}QNode;

static QNode *head, *tail;
pid_t pid = 0;
PCB pcbArray[18];
resourceDesc resourceArray[20];

void createSharedMemKeys(key_t *resourceKey, key_t *timeKey, key_t *pcbKey){
	*timeKey = ftok(".", 2820);
	*pcbKey = ftok(".", 4378);
	*resourceKey = ftok(".", 8564);
};

void createSharedMemory(int *timeid, int *pcbid, int *resourceid, key_t timeKey, key_t pcbKey, key_t resourceKey){
	*timeid = shmget(timeKey, (sizeof(unsigned int) * 2), 0666 | IPC_CREAT);
	*pcbid = shmget(pcbKey, STRUCT_ARRAY_SIZE, 0666 | IPC_CREAT); //Grayson, tomorrow finish implementing the pcb
	*resourceid = shmget(resourceKey, (sizeof(unsigned int) * 2), 0666 | IPC_CREAT);
	if(*timeid == -1){
		printf("timeid %s\n", strerror(errno));		
	}
	if(*pcbid == -1){
		printf("pcbid %s\n", strerror(errno));		
	}
	if(*resourceid == -1){
		printf("resourceid %s\n", strerror(errno));		
	}
};

void attachToSharedMemory(unsigned int **seconds, unsigned int **nanoseconds, PCB **pcbPtr, resourceDesc **resourcePtr, int timeid, int pcbid, int resourceid){
	*seconds = (unsigned int*)shmat(timeid, NULL, 0);
	if(*seconds == -1){
		printf("seconds %s\n", strerror(errno));	
	}
	*nanoseconds = *seconds + 1;
	if(*nanoseconds == -1){
		printf("nanoseconds %s\n", strerror(errno));	
	}
	*pcbPtr = (PCB*)shmat(pcbid, NULL, 0);
	if(*pcbPtr == -1){
		printf("pcbptr %s\n", strerror(errno));	
	}
	*resourcePtr = (resourceDesc*)shmat(resourceid, NULL, 0);
	if(*resourcePtr == -1){
		printf("resourceptr %s\n", strerror(errno));	
	}
};

void createArgs(char *sharedTimeMem, char *sharedPCBMem, char *sharedPositionMem, char *sharedResourceMem, int timeid, int pcbid,int resourceid, int position){
	snprintf(sharedTimeMem, sizeof(sharedTimeMem)+25, "%d", timeid);
	snprintf(sharedPCBMem, sizeof(sharedPCBMem)+25, "%d", pcbid);
	snprintf(sharedPositionMem, sizeof(sharedPositionMem)+25, "%d", position);
	snprintf(sharedResourceMem, sizeof(sharedResourceMem)+25, "%d", resourceid);
};

void initializeUser(unsigned int **seconds, unsigned int **nanoseconds, PCB *pcbPtr, int position){
	srand(time(NULL));
	pcbPtr[position].pid = getpid();
	
	//printf("Current time %u %u\n", *seconds, *nanoseconds);
};

void forkChild(char *sharedTimeMem, char *sharedPCBMem, char *sharedPositionMem, char*sharedResourceMem, unsigned int *seconds, unsigned int *nanoseconds, int *position){
	if((pid = fork()) == 0){
		execlp("./user", "./user", sharedTimeMem, sharedPCBMem, sharedPositionMem,sharedResourceMem, NULL);
	}
	//printf("position in forkchild: %d\n", *arrayPosition);
	
};

int resourceAllocation(PCB *pcbPtr, resourceDesc *resource, int position){
	//should i be giving it one at a time? could have a while loop in here that gives it the resources until the requirement one at a time.
	//if i go through each resource one by one whats the point of giving a resource anything elase if i cant give it one. 
	//should actually be subtracting requirement
	//if resource requested is more than its limit
	//if resource requested is more than whats available
	int i;
	//im confused as to what the resource requirements are in relation. will it ever request more than that? can it request more than what is started with? NO!
	for(i = 0; i < 	20; i++){
		if(pcbPtr[position].resourceRequirements[i] > 0){	
			if((pcbPtr[position].resourcesAllocated[i] + pcbPtr[position].resourceRequirements[i]) > pcbPtr[position].resourceLimits[i]){ //checking if its asking for more than its limit
				printf("P%d wanted more than its limit %d of resource R%d\n", getpid(), pcbPtr[position].resourceLimits[i], i); 		
				//return 1;
			} else if(resource[i].resources < pcbPtr[position].resourceRequirements[i]){ //checking if it shouold block cause process aksing more than resource currently has
				pcbPtr[position].waitingToBlock = 1; //it will be blocked in oss later
				printf("P%d is being blocked because it needed %d resources from R%d and it only had %d.\n", position, pcbPtr[position].resourceRequirements[i], i, resource[i].resources);
				return 0; 
			} else { //otherwise give it the thing.
				printf("P%d is requesting %d resources from R%d", position, pcbPtr[position].resourceRequirements[i], i);
				resource[i].resources -= pcbPtr[position].resourceRequirements[i]; //available - request
				pcbPtr[position].resourcesAllocated[i] += pcbPtr[position].resourceRequirements[i]; //allocation + request
				pcbPtr[position].resourceRequirements[i] -= pcbPtr[position].resourceRequirements[i]; // need - request;
				printf(" bringing R to %d from max of %d\n", resource[i].resources, resource[i].max);
			}
		}	
	}
	return 1;
}
	
void resourceRelease(PCB *pcbPtr, resourceDesc *resource, int position){
	//first i have to check if I can actually release what I want to release
	int i;
	for(i = 0; i < 20; i++){
		if(pcbPtr[position].resourceRequirements[i] > 0){
			if(pcbPtr[position].resourcesAllocated[i] < pcbPtr[position].resourceRequirements[i]){ //can it release more than it asked for?
				printf("canny do that m8\n");				
				return;
			} else {
				printf("P%d is releasing %d resources from R%d", position, pcbPtr[position].resourceRequirements[i], i);
				resource[i].resources += pcbPtr[position].resourceRequirements[i]; //release it				
				pcbPtr[position].resourcesAllocated[i] -= pcbPtr[position].resourceRequirements[i]; //decrement process allocated
				pcbPtr[position].resourceRequirements[i] -= pcbPtr[position].resourceRequirements[i]; //decrement process requirement
			}
		}
	}
}
void unblockProcess(PCB *pcbPtr, resourceDesc *resourcePtr, int position){
	int i;
	for(i = 0; i < 20; i++){
		if(pcbPtr[position].resourceRequirements[i] <= resourcePtr[i].resources){
			resourceAllocation(pcbPtr, resourcePtr, position);
			pcbPtr[position].waitingToBlock = 0;
			return;
		}	
	}
}
void clearPcb(PCB *pcbPtr, resourceDesc *resourcePtr, int position){
	int i;
	for(i = 0; i < 20; i++){
		resourcePtr[i].resources += pcbPtr[position].resourcesAllocated[i]; //release it	
		pcbPtr[position].resourceLimits[i] = 0;		
		pcbPtr[position].resourceRequirements[i] = 0;
		pcbPtr[position].resourcesAllocated[i] = 0;
	}
	pcbPtr[position].waitingToBlock = 0;
	pcbPtr[position].position = 0;
	pcbPtr[position].isSet = 0;
	pcbPtr[position].pid = 0;	
}
//For the queues

int isBlockedQueueEmpty(){
	if(head == NULL){
		printf("HEAD EMPTY\n");
		return 1;
	} else {
		printf("HEAD FULL\n");
		return 0;
        }
};


int isBlockedQueueFull(){
        return FALSE;
};

void initBlockedQueue(){
	head = tail = NULL;
};

void clearBlockedQueue(){
	QNode *temp;
	while(head != NULL){
		temp = head;
		head = head->next;
		free(temp);
	}
	head = tail = NULL;
};

int enqueueBlocked(PCB *sptr, int position){
	 QNode *temp;
        if(isBlockedQueueFull()) return FALSE;
	printf("ENQUEUEING Blocked %d\n", sptr[position].pid);
        temp = (QNode *)malloc(sizeof(QNode));
        temp -> qptr = &sptr[position];
        temp -> next = NULL;
        if(head == NULL){
                head = tail = temp;
        } else {
                tail -> next = temp;
                tail = temp;
        }
        return TRUE;
};

pid_t dequeueBlocked(PCB *sptr, int *position){	
	printf("DEQUEUEING Blocked %d\n", sptr[*position].pid);
	
	QNode *temp;
        if(isBlockedQueueEmpty()){
                return FALSE;
        } else {
                *position = head->qptr[0].position;
	        printf("POSITION Blocked: %d\n", *position);
		sptr[*position] = head->qptr[0];
		temp = head;
		head = head->next;
                free(temp);

                if(isBlockedQueueEmpty()){
                        head = tail = NULL;
                }
        }
        return sptr[*position].pid;
};
 
void enqueueBlockedProcess(PCB *sptr){
	int i;
	for(i = 0; i < 18; i++){
		if(sptr[i].waitingToBlock == 1){
			enqueueBlocked(sptr, i);
		}
	}
};
// end of queue funcs
/*int resourceAllocation(PCB *pcbPtr, resourceDesc *resource, int position, int resourceRequested, int amount){
	//should i be giving it one at a time? could have a while loop in here that gives it the resources until the requirement one at a time.

	//im confused as to what the resource requirements are in relation. will it ever request more than that? can it request more than what is started with? NO!
	printf("P%d wants %d from R%d\n", position, amount, resourceRequested);
	if((pcbPtr[position].resourcesAllocated[resourceRequested] + amount) > pcbPtr[position].resourceLimits[resourceRequested]){ //checking if its asking for more than its limit
		printf("P%d wanted more than its limit %d\n", getpid(), pcbPtr[position].resourceLimits[resourceRequested]); 		
		return 1;
	} else if(resource[resourceRequested].resources < amount){ //checking if it shouold block cause process aksing more than resource currently has
		pcbPtr[position].resourceRequirements[resourceRequested] += amount;
		pcbPtr[position].waitingToBlock = 1; //it will be blocked in oss later
		printf("P%d is being blocked because it needed %d resources from R%d and it only had %d.\n", position, amount, resourceRequested, resource[resourceRequested].resources);
		return 0; 
	} else { //otherwise give it the thing.
		if(pcbPtr[position].resourcesAllocated[resourceRequest] == pcbPtr[position].resourceRequirements[resourceRequested]){	
			pcbPtr[position].resourceRequirements[resourceRequested] += amount; //If the total allocated is equal to the requirement and it wants more.
		}
		resource[resourceRequested].resources -= amount;
		pcbPtr[position].resourcesAllocated[resourceRequested] += amount;
		printf("P%d is requesting %d resources from R%d bringing it to %d from max of %d\n", position, amount, resourceRequested, resource[resourceRequested].resources, resource[resourceRequested].max);
	}	
	return 1;
}*/


