 
/***
Simple program to illustrate the use of pthreads (Creation, Joining, Exit, Mutex)

Assuming threads are created as JOINABLE by default
***/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_THREADS 5

/***
Just a data structure to store the number of a thread (Thread 0, 1, ...) 
***/

struct thread_data {
	int t_index;
};

int global_count = 0;


/***
Types defined in pthread.h>
***/

pthread_t thread_id[NUM_THREADS];
pthread_mutex_t mutex1;


// Function executed by all threads created
void *thread_start(void *param)
{
	// Take out the parameter
	struct thread_data *t_param = (struct thread_data *) param;
	int tid = (*t_param).t_index;

	// Update global count inside critical section
	pthread_mutex_lock(&mutex1);
	global_count++;
	pthread_mutex_unlock(&mutex1);

	// Print success and exit. Actually should not print 
	// thread id with %ld, but ok here as the types match in Linux
	printf("Thread  %ld finished updating\n", thread_id[tid]);
	pthread_exit(NULL);
}


int main()
{
	int no_of_threads, i, id;
	struct thread_data param[NUM_THREADS];
	
	// initialize mutex
	pthread_mutex_init(&mutex1, NULL);

	// Create the threads
	for(i=0; i<NUM_THREADS; i++)
	{
		 param[i].t_index = i;
		 pthread_create(&thread_id[i], NULL, thread_start, (void *) &param[i]);
	}

	// Wait for all threads to terminate
	for(i=0; i<NUM_THREADS; i++)
	{
		pthread_join(thread_id[i], NULL);
	}

	// The lock is actually unnecessary here as all other threads
	// must have exited. But just good practice; in a general setting
	// there can be other threads accessing it other than the ones
	// this process waits for

	pthread_mutex_lock(&mutex1);
	printf("Final Count is %d\n", global_count);
	pthread_mutex_unlock(&mutex1);

	// Clean up the mutex
	pthread_mutex_destroy(&mutex1);
} 

	
	
	 

