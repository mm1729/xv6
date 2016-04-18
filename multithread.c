
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "pthread.h"


#define NUM_THREADS 17
#define TARGET_COUNT_PER_THREAD 100000


pthread_t threads[NUM_THREADS];
void *lol(void *arg){
	int i;
	int counter;
	sleep(10);
	printf(1,"RUNNNING!\n");
	//printf(1, "thread %d: started...\n", *(int*)arg);

	for (i=0; i<TARGET_COUNT_PER_THREAD; i++) {
		sleep(0);
		counter++;
		sleep(0);
	}

	//int * oop = malloc(4);
	//*oop=1000;
	//printf(1,"Exiting\n");
	//printf(1,"%d\n",*(int*)arg);
	pthread_exit(arg);
}

int j= 16;
void *thread(void *arg)
{
	int i;
	int counter;
	int toJoin;

	sleep(10);
	printf(1, "thread %d: started...\n", *(int*)arg);

		//int* wtf = (int*) malloc(sizeof(int));
		//*wtf=20;
		if(*(int*)arg == 0){
		toJoin = j++;
		printf(1,"I created %d, I am %d\n",toJoin,*(int*)arg);
		pthread_create(&threads[16], 0,lol, 0);
		}
		//pthread_create(&threads[toJoin], 0,lol, 0);



	for (i=0; i<TARGET_COUNT_PER_THREAD; i++) {
		sleep(0);
		counter++;
		sleep(0);
	}

	//void *retval;
	printf(1,"I joined %d, I am %d\n",toJoin,*(int*)arg);
	if(*(int*)arg ==0){
		void *retval;
	int ret = pthread_join(threads[16], &retval);
	printf(1,"joinstatus%d\n",ret);
}
	//printf(1,"ret%d\n",*(int*)retval);

	pthread_exit(arg);
}

int main(int argc, char **argv)
{
	int i;
	int passed = 1;

	// Set up thread stuff
	// Threads

	// Args
	int *args[NUM_THREADS];

	// Allocate stacks and args and make sure we have them all
	// Bail if something fails
	for (i=0; i<16; i++) {
		args[i] = (int*) malloc(sizeof(int));
		if (!args[i]) {
			printf(1, "main: could not get memory (for arg) for thread %d, exiting...\n");
			exit();
		}

		*args[i] = i;
	}

	printf(1, "main: running with %d threads...\n", NUM_THREADS);

	// Start all children
	for (i=0; i<16; i++) {
		pthread_create(&threads[i], 0, thread, args[i]);
		printf(1, "main: created thread with pid %d\n", threads[i].pid);
	}

	// Wait for all children
	for (i=0; i<16; i++) {
		void * retval;
		int r;
		r = pthread_join(threads[i], &retval);
		if (r < 0) {
			passed = 0;
		}
		if (*(int*)retval != i) {
			passed = 0;
		}
		printf(1, "main: thread %d joined...retval=%d\n", i, *(int*)retval);
	}

	if (passed) {
		printf(1, "TEST PASSED!\n");
	}
	else {
		printf(1, "TEST FAILED!\n");
	}

	// Clean up memory
	for (i=0; i< 16; i++) {
		free(args[i]);
	}


	// Exit
	exit();
}
