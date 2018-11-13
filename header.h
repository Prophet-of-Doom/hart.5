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


typedef struct QNodeType{
	//PCB *qptr;
	struct QNodeType *next;
	//struct QNodeType *currentNode;
}QNode;

//static QNode *headLP, *tailLP, *headHP, *tailHP;
pid_t pid = 0;
//message *msg = NULL;

void createSharedMemKeys( key_t *timeKey){
	
	*timeKey = ftok(".", 'C');
};

void createSharedMemory( int *timeid, key_t timeKey){
	
	*timeid = shmget(timeKey, (sizeof(unsigned int) * 2), 0666 | IPC_CREAT);
};

void attachToSharedMemory( unsigned int **seconds, unsigned int **nanoseconds, int timeid){
	
	*seconds = (unsigned int*)shmat(timeid, NULL, 0);
	*nanoseconds = *seconds + 1;
	//printf("seconds %u\n", **seconds);
};

void createArgs( char *sharedTimeMem, int timeid){
	//snprintf(sharedMsgMem, sizeof(sharedMsgMem)+2, "%d", msgid); 
	snprintf(sharedTimeMem, sizeof(sharedTimeMem)+2, "%d", timeid);
};

void initializeUser( unsigned int **seconds, unsigned int **nanoseconds){
	srand(time(NULL));
	//printf("USER PID %d\n", getpid());
	//printf("Current time %u %u\n", *seconds, *nanoseconds);
};

void forkChild(char *timeMem, unsigned int *seconds, unsigned int *nanoseconds){
	if((pid = fork()) == 0){
		//printf("OSS: Created a user at %u.%u\n", *seconds, *nanoseconds);
		execlp("./user", "./user", timeMem, NULL);
	}
	//printf("position in forkchild: %d\n", *arrayPosition);
	
};

//For the queues
/*
void enqueueReadyProcess(PCB *sptr){
	int i;
	for(i = 0; i < 18; i++){
		if(sptr[i].waitingToQueue == 0){
			if(sptr[i].isSet == 1){
				if(sptr[i].processPriority == 1){
					enqueueHP(sptr, i);
			
				} else {
					enqueueLP(sptr, i);
				}	
			}
		}
	}
};



int isLPQueueEmpty(){
	if(headLP == NULL){
		printf("HEAD LP EMPTY\n");
		return 1;
	} else {
		printf("HEAD LP FULL\n");
		return 0;
        }
};

int isHPQueueEmpty(){
	if(headHP == NULL){
		printf("HEAD HP EMPTY\n");
		return 1;
	}else{
		printf("HEAD HP FULL\n");
		return 0;	
	}
};

int isLPQueueFull(){
        return FALSE;
};

int isHPQueueFull(){
        return FALSE;
};

void initLPQueue(){
	headLP = tailLP = NULL;
};

void initHPQueue(){
	headHP = tailHP = NULL;
};

void clearLPQueue(){
	QNode *temp;
	while(headLP != NULL){
		temp = headLP;
		headLP = headLP->next;
		free(temp);
	}
	headLP = tailLP = NULL;
};

void clearHPQueue(){
	QNode *temp;
        while(headHP != NULL){
                temp = headHP;
                headHP = headHP->next;
                free(temp);
        }
        headHP = tailHP = NULL;
};

int enqueueHP(PCB *sptr, int position){
	QNode *temp;
	if(isHPQueueFull()) return FALSE;
	printf("ENQUEUEING HP %d\n", sptr[position].pid);
	temp = (QNode *)malloc(sizeof(QNode));
	temp -> qptr = &sptr[position];
	temp -> next = NULL;
	if(headHP == NULL){
		headHP = tailHP = temp;
	} else {
		tailHP -> next = temp;
		tailHP = temp;
	}
		
	QNode *currentNode = headHP;
	while(currentNode != NULL){
		printf("user: in the queue %d -- \n", currentNode->qptr[0].pid);
		currentNode = currentNode->next;
	}
	return TRUE; 
};

int enqueueLP(PCB *sptr, int position){
	 QNode *temp;
        if(isLPQueueFull()) return FALSE;
	printf("ENQUEUEING LP %d\n", sptr[position].pid);
        temp = (QNode *)malloc(sizeof(QNode));
        temp -> qptr = &sptr[position];
        temp -> next = NULL;
        if(headLP == NULL){
                headLP = tailLP = temp;
        } else {
                tailLP -> next = temp;
                tailLP = temp;
        }
        return TRUE;
};

pid_t dequeueHP(PCB *sptr, int *position){
	printf("DEQUEUEING HP %d\n", sptr[*position].pid);
	QNode *temp;
	if(isHPQueueEmpty()){
		return FALSE;
	} else {
		*position = headHP->qptr[0].position;
		printf("POSITIONHP: %d\n", *position);
		sptr[*position] = headHP->qptr[0];
		temp = headHP;
		headHP = headHP->next;
	
		free(temp);
		
		if(isHPQueueEmpty()){
			headHP = tailHP = NULL;
		}
	}
	return sptr[*position].pid;
};

pid_t dequeueLP(PCB *sptr, int *position){	
	printf("DEQUEUEING LP %d\n", sptr[*position].pid);
	
	QNode *temp;
        if(isLPQueueEmpty()){
                return FALSE;
        } else {
                *position = headLP->qptr[0].position;
	        printf("POSITIONLP: %d\n", *position);
		sptr[*position] = headLP->qptr[0];
		temp = headLP;
		headLP = headLP->next;
                free(temp);

                if(isLPQueueEmpty()){
                        headLP = tailLP = NULL;
                }
        }
        return sptr[*position].pid;
};
 */

// end of queue funcs
