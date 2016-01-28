#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *howdy(void *arg){
	long tid;
	tid=(long) arg;
	printf("Howdy %d\n",tid);
}

int main(){
#define NTHREADS 20;

	int threadid;
	pthread_t threads[NTHREADS];
	for(threadid=0;threadid<NTHREADS;threadid++){
		pthread_creat(&threads[threadid], NULL, howdy, (void*)threadid);
	}
	pthread_exit(NULL);
}
