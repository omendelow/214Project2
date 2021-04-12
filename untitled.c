#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>

#ifndef DEBUG
#define DEBUG 0
#endif

//max filepath length
int QUEUESIZE = 100;

struct queue 
{
    char** data;
    unsigned head;  // index of first item in queue
    unsigned count;  // number of items in queue
    pthread_mutex_t lock;
    pthread_cond_t read_ready;  // wait for count > 0
    pthread_cond_t write_ready; // wait for count < QUEUESIZE
};

int queue_init(struct queue *Q)
{
	Q-> data = malloc(sizeof(char) * QUEUESIZE * QUEUESIZE);
    Q->head = 0;
    Q->count = 0;
    int err = pthread_mutex_init(&Q->lock, NULL);
    pthread_cond_init(&Q->read_ready, NULL);
    pthread_cond_init(&Q->write_ready, NULL);
    
    return err;  // obtained from the init functions (code omitted)
}

int queue_add(struct queue *Q, char* item)
{
    pthread_mutex_lock(&Q->lock); // make sure no one else touches Q until we're done
    
    while (Q->count == QUEUESIZE) {
        // wait for another thread to dequeue
        pthread_cond_wait(&Q->write_ready, &Q->lock);
            // release lock & wait for a thread to signal write_ready
    }
    
        // at this point, we hold the lock & Q->count < QUEUESIZE
    
    unsigned index = Q->head + Q->count;
    if (index >= QUEUESIZE) index -= QUEUESIZE;
    
    Q->data[index] = item;
    ++Q->count; 
    
    pthread_mutex_unlock(&Q->lock); // now we're done
    pthread_cond_signal(&Q->read_ready); // wake up a thread waiting to read (if any)

    return 0;
}

int queue_remove(struct queue *Q, char **item)
{
    pthread_mutex_lock(&Q->lock);
    
    while (Q->count == 0) {
        pthread_cond_wait(&Q->read_ready, &Q->lock);
    }
    
        // now we have exclusive access and queue is non-empty
    
    *item = Q->data[Q->head];  // write value at head to pointer
    --Q->count;
    ++Q->head;
    if (Q->head == QUEUESIZE) Q->head = 0;
    
    pthread_mutex_unlock(&Q->lock);
    
    //wasn't sure what to return
    return pthread_cond_signal(&Q->write_ready);
}

void queue_print(struct queue *Q) 
{
	for (int i = 0; i < Q->count; i++) {
		printf("%s%s", Q->data[i], (i == Q->count - 1 ? "\n" : ", "));
	}
}

int main(int argc, char **argv)
{
	// Check number of arguments passed
	if (argc < 2)
	{
		// no arguments were passed
		perror("Error: No arguments passed.");
		exit(EXIT_FAILURE);
	}
	struct queue file_list;
	queue_init(&file_list);
	for (int i = 1; i < argc; i++) {
		queue_add(&file_list, argv[i]);
	}
	queue_print(&file_list);
	//process_arguments(argc, argv, &files);
	//al_print(&L);
	//al_destroy(&L);
}
