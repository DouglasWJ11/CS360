#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <queue>

#define THREADS 10
using namespace std;

//sem_wait increments the counter
//sem_post decrements the counter

queue<int> work_tasks; //All the tasks 

sem_t work_mutex; //Controls queues. Makes sure only one task is moving at a time 
sem_t work_to_do; //Tells us how many tasks are on the queue
sem_t space_on_q; // Tells us if there is any space on the queue

struct thread_info
{
    int thread_id;
    int another_number;
};

void* serve( void* in_data )
{
    struct thread_info* t_info = ( struct thread_info* ) in_data;
    int tid = t_info->thread_id;

    while( 1 )
    {
	sem_wait( &work_to_do); //Is there work to do? Okay, then I will do it. If not we're just waiting
        sem_wait( &work_mutex ); //I'm locking myself so that no one else can touch me.
        
        int work_object_grabbed = work_tasks.front(); //Take the first thing in the work queue
        work_tasks.pop(); //Pop it from the queue
       
        cout << "I am thread: " << tid << "\t";
	cout << "I grabbed: " << work_object_grabbed << endl; 
       
	//read from the file descriptor (work_object_grabbed)
	//parse the headers
	//find the resource
        //    check if folder or file
	//    Get file contents
	//    get file length
	//    get file extensions for http response
        // format your http response (headers and body)
	// write response you requested

        sem_post( &work_mutex ); //Alright, I'm done. Freeing myself up
        sem_post( &space_on_q ); //We can add one to the space, we finished. 
        
    }
}

int main( int argv, char* argc[] )
{
    std::cout << "This is an example of pthreads!" << std::endl;

    sem_init( &work_mutex, 0, 1 ); //Only one thread can access this, protects the mutual exclusion - The queue will work with this
    sem_init( &work_to_do, 0, 0);
    sem_init( &space_on_q, 0, 100);

    pthread_t threads[ THREADS ];

    struct thread_info all_thread_info[ THREADS ];

	//Creating a thread pool
    for( int i = 0; i < THREADS; i++ )
    {
        sem_wait( &work_mutex );
 
        std::cout << "creating thread: " << i << "\t" << std::endl;
        all_thread_info[ i ].thread_id = i;
        pthread_create( &threads[ i ], NULL, serve, ( void* ) &all_thread_info[ i ] );
 
        sem_post( &work_mutex );
    }
    
    //create a socket sockfd
    //bind socket to port number
    // tell socket to listen

    int counter = 0;
    while( 1 )
    {
        // spin your wheels
        //web server* will add requests to work tasks here
        //*producers
        // tasks are file descriptors returned by accept()
        //Putting as much stuff onto the work queue as possible
    
        //socket accept()s connections, returns file descriptor

        sem_wait( &space_on_q);  //"Yes I have enough space on the queue"
        sem_wait( &work_mutex);  //"Now I need to lock the queue so that nobody tries to take this until i'm done"
        
        work_tasks.push(counter); 
        
        sem_post( &work_mutex); //
        sem_post( &work_to_do); //There is now work to do, let's go do it
        counter++;
    }

    return 0;
}

