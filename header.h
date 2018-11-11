#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/msg.h>
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
	int waitingToQueue;
	int position;
	int isSet;
	int resourceLimits[20];
	int resourceRequirements[20];
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
	*timeKey = ftok(".", 'C');
	*pcbKey = ftok(".", 'H');
	*resourceKey = ftok(".", 'D');
};

void createSharedMemory(int *timeid, int *pcbid, int *resourceid, key_t timeKey, key_t pcbKey, key_t resourceKey){
	*timeid = shmget(timeKey, (sizeof(unsigned int) * 2), 0666 | IPC_CREAT);
	*pcbid = shmget(pcbKey, STRUCT_ARRAY_SIZE, 0666 | IPC_CREAT); //Grayson, tomorrow finish implementing the pcb
	*resourceid = shmget(resourceKey, (sizeof(unsigned int) * 2), 0666 | IPC_CREAT);
};

void attachToSharedMemory(unsigned int **seconds, unsigned int **nanoseconds, PCB **pcbPtr, resourceDesc **resourcePtr, int timeid, int pcbid, int resourceid){
	*seconds = (unsigned int*)shmat(timeid, NULL, 0);
	*nanoseconds = *seconds + 1;
	*pcbPtr = (PCB*)shmat(pcbid, NULL, 0);
	*resourcePtr = (resourceDesc*)shmat(resourceid, NULL, 0);
};

void createArgs(char *sharedTimeMem, char *sharedPCBMem, char *sharedPositionMem, char *sharedResourceMem, int timeid, int pcbid,int resourceid, int position){
	snprintf(sharedTimeMem, sizeof(sharedTimeMem)+2, "%d", timeid);
	snprintf(sharedPCBMem, sizeof(sharedPCBMem)+2, "%d", pcbid);
	snprintf(sharedPositionMem, sizeof(sharedPositionMem)+2, "%d", position);
	snprintf(sharedResourceMem, sizeof(sharedResourceMem)+2, "%d", resourceid);
};

void initializeUser(unsigned int **seconds, unsigned int **nanoseconds, PCB *pcbPtr){
	srand(time(NULL));
	printf("USER PID %d\n", getpid());
	//printf("Current time %u %u\n", *seconds, *nanoseconds);
};

void forkChild(char *sharedTimeMem, char *sharedPCBMem, char *sharedPositionMem, char*sharedResourceMem, unsigned int *seconds, unsigned int *nanoseconds, int *position){
	if((pid = fork()) == 0){
		
		execlp("./user", "./user", sharedTimeMem, sharedPCBMem, sharedPositionMem,sharedResourceMem, NULL);
	}
	//printf("position in forkchild: %d\n", *arrayPosition);
	
};

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
		if(sptr[i].waitingToQueue == 0){
			if(sptr[i].isSet == 1){
				enqueueBlocked(sptr, i);
			}
		}
	}
};
// end of queue funcs
