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

int main(int argc, char *argv[]){
	srand(time(NULL));
	alrm = 0;
	
	char* filename = malloc(sizeof(char));
	filename = "log.txt";
        FILE *logFile = fopen(filename, "a");

	key_t msgKey = ftok("progfile", 65), timeKey = 0; 
	int timeid = 0, msgid = msgget(msgKey, 0666 | IPC_CREAT); 
	unsigned int *seconds = 0, *nanoseconds = 0;
		
	char sharedTimeMem[10];
	
	pid_t pid = 0;
	
        createSharedMemKeys( &timeKey);
        createSharedMemory( &timeid, timeKey);
	attachToSharedMemory( &seconds, &nanoseconds, timeid);
	
      	//initLPQueue();
	//initHPQueue();
 	message.mesg_type = 1; 
	int forked = 0;
	//signal(SIGALRM, timerKiller);
        //alarm(2);
createArgs(sharedTimeMem, timeid);
forkChild(sharedTimeMem, seconds, nanoseconds);
	do{
		//sprintf(message.mesg_text, "OSS IS SENDING\n");
		msgsnd(msgid, &message, sizeof(message), 0);
		
		
		//sleep(2);
		//forked++;
		//*seconds += 1;
		//*nanoseconds += 100000;
		//printf("IN OSS current seconds: %u, %u\n", *seconds, *nanoseconds);
		msgrcv(msgid, &message, sizeof(message), 1, 0);
		printf("Data Received is : %s \n",  message.mesg_text); 
	}while((forked < 25) && alrm == 0);
	printf("OSS: OUT OF LOOP\n");	
	msgctl(msgid, IPC_RMID, NULL);
 	fclose(logFile);
        //shmdt(msg);
	shmdt(seconds);
        shmctl(msgid, IPC_RMID, NULL);
	shmctl(timeid, IPC_RMID, NULL);
	kill(0, SIGTERM);
	return 0;
}
