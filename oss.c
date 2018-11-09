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


int alrm;
void timerKiller(int sign_no){
        alrm = 1;
}

int main(int argc, char *argv[]){
	srand(time(NULL));
	alrm = 0;
	
	char* filename = malloc(sizeof(char));
	filename = "log.txt";
        FILE *logFile = fopen(filename, "a");

	key_t msgKey = 0, timeKey = 0; 
	int msgid = 0, timeid = 0;
	unsigned int *seconds = 0, *nanoseconds = 0;
	message *msg = NULL;	
	char sharedMsgMem[10], sharedTimeMem[10];
	
	pid_t pid = 0;
	
        createSharedMemKeys(&msgKey, &timeKey);
        createSharedMemory(&msgid, &timeid, msgKey, timeKey);
	attachToSharedMemory(&msg, &seconds, &nanoseconds, msgid, timeid);

      	//initLPQueue();
	//initHPQueue();
 	
	int forked = 0;
	//signal(SIGALRM, timerKiller);
        //alarm(2);
	/*do{
		
	}while((forked < 100) && alrm == 0); */
	printf("OSS: OUT OF LOOP\n");	
	
 	fclose(logFile);
        shmdt(msg);
	shmdt(seconds);
        shmctl(msgid, IPC_RMID, NULL);
	shmctl(timeid, IPC_RMID, NULL);
	kill(0, SIGTERM);
	return 0;
}
