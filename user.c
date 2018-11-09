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
 
#include "header.h"
 
int main(int argc, char *argv[]) { 	
        int msgid = atoi(argv[1]);
	int timeid = atoi(argv[2]);
	pid_t pid = getpid();
	unsigned int *seconds = 0, *nanoseconds = 0;
        message *msg = NULL;

 	shmdt(seconds);
     	shmdt(msg);
	return 0;
}
