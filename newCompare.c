#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>

typedef struct Node
{
	char *word;
	int count;
	double frequency;
	struct Node *next;
}Node;

typedef struct repoNode
{
	char* filename;
	int numWords;
	Node *WFD;
	struct repoNode *next;
	
}repoNode;

//Set head of WFD repository to NULL
repoNode *repoHead = NULL;

typedef struct strbuf_t
{
    int length;
    int used;
    char *data;
} strbuf_t;

int sb_init(strbuf_t *sb, size_t length) {
    if (sb == NULL || length < 1) return 1;

    sb->data = malloc(sizeof(char) * length);
    if (!sb->data) return 1;

    sb->length = length;
    sb->used = 1;

    sb->data[0] = '\0';
    return 0;
}

void sb_destroy(strbuf_t *sb) {
    free(sb->data);
    sb->used = 0;
    sb->length = 0;
}

int sb_append(strbuf_t *sb, char item) {
    if (sb->used == sb->length) {
        sb->length *= 2;
        char *p = realloc(sb->data, sizeof(char) * sb->length);
        if (!p) return 1;
        sb->data = p;
    }

    sb->data[sb->used - 1] = item;
    sb->data[sb->used++] = '\0';
    return 0;
}

Node *front = NULL;
int numWords = 0;

void insert(char* word, int size) {
    numWords++;

    if (front == NULL) {
        front = malloc(sizeof(Node));
        front->next = NULL;
        char *newWord = malloc((size)*sizeof(char));
        newWord[size-1] = '\0';
        memcpy(newWord, word, size);
        front->word = newWord;
        front->count = 1;
        return;
    }
    
    Node *ptr = front;
    Node *prev = NULL;

    while (ptr != NULL) {
        //word is already in the list
        if (strcmp(word, ptr->word) == 0) {
            ptr->count++;
            return;
        }
        //word is lexicographically less than word in list
        else if (strcmp(word, ptr->word) < 0) {
            Node *newNode = malloc(sizeof(Node));
            newNode->next = NULL;
            char *newWord = malloc((size)*sizeof(char));
            newWord[size-1] = '\0';
            memcpy(newWord, word, size);
            newNode->word = newWord;
            newNode->count = 1;
            if (prev == NULL) {
                newNode->next = ptr;
                front = newNode;
                return;
            }
            prev->next = newNode;
            newNode->next = ptr;
            return;
        }
        else {
            prev = ptr;
            ptr = ptr->next;
        }
    }
    //word is lexicographically greater than word in list
    Node *newNode = malloc(sizeof(Node));
    char *newWord = malloc((size)*sizeof(char));
    newWord[size-1] = '\0';
    memcpy(newWord, word, size);
    newNode->word = newWord;
    newNode->count = 1;
    prev->next = newNode;
    newNode->next = NULL;
    return;
}

void getWords(int fd) {

    strbuf_t *sb = malloc(sizeof(strbuf_t));
    sb_init(sb, 1);

    int bytes;
    int size = 10*sizeof(char); //buffer size
    char *buffer = malloc(size);
    int end = 0; //ending index of a word

    while ((bytes = read(fd, buffer, size)) > 0) {

        end = 0;

        //sb has a word and the first element of buffer is WS  
        if (isspace(buffer[end]) && sb->used > 1) {
            insert(sb->data, sb->used);
            sb_destroy(sb);
            end++;
        }
        
        //sb has a word and the first element of buffer is not WS  
        else if (!isspace(buffer[end]) && sb->used > 1) {
            while (!isspace(buffer[end])) {
                if (isalpha(buffer[end]) || isdigit(buffer[end]) || buffer[end] == '-') {
                    char c = tolower(buffer[end]);
                    sb_append(sb, c);
                }
                end++;
                if (end >= bytes) {
                    break;
                }
            }   
        }
        
        while (end != bytes) {
            while (!isspace(buffer[end])) {
                if (isalpha(buffer[end]) || isdigit(buffer[end]) || buffer[end] == '-') {
                    if (sb->length < 1) {
                        sb_init(sb, 1);
                    }
                    char c = tolower(buffer[end]);
                    sb_append(sb, c);
                }
                end++;
                if (end >= bytes) {
                    break;
                }
            }
            if (end >= bytes) {
                break;
            }
            if (sb->length > 1) {
                insert(sb->data, sb->used);
                sb_destroy(sb);
            }
            end++;
            if (end >= bytes) {
                break;
            }
        }
    }
    free(buffer);
    free(sb);
}

void getFrequencies(Node *front) {
    Node *curr = front;
   	while (curr != NULL) {
        curr->frequency = (double) curr->count/numWords;
        curr = curr->next;
    }
}

void printList(repoNode *head) {
    repoNode *curr = head;
	while (curr != NULL) {
		printf("\n[File: %s, numWords: %d] ", curr->filename, curr->numWords);
		Node *ptr = curr->WFD;
   		while (ptr != NULL) {
        	printf("%s, %d, %f-> ", ptr->word, ptr->count, ptr->frequency);
        	ptr = ptr->next;
    	}
		curr = curr->next;
	}
	printf("\n");
}

void cleanUp(repoNode *head) {
    repoNode* curr = head;
    while (curr != NULL) {
		Node *ptr = curr->WFD;
		while (ptr != NULL) {
			Node* temp = ptr;
			ptr = ptr->next;
			free(temp->word);
			free(temp);
		}
		repoNode *temp = curr;
		curr = curr->next;
		free(temp->filename);
		free(temp);
    }
	front = NULL;
	repoHead = NULL;
}

typedef struct QNode 
{
    char* data;
    struct QNode *next;
}QNode;
  
typedef struct Queue 
{
    QNode *head;  // first item in Queue
    QNode *rear;  // last item in Queue
    int count;  // number of items in Queue
	int activeThreads; // number of active Threads
    pthread_mutex_t lock;
    pthread_cond_t read_ready;  // wait for count > 0
    // pthread_cond_t write_ready; // wait for count < QUEUESIZE
}Queue;

int queue_init(Queue *Q)
{
    Q->head = NULL;
    Q->rear = NULL;
    Q->count = 0;
    int err = pthread_mutex_init(&Q->lock, NULL);
    pthread_cond_init(&Q->read_ready, NULL);
    // pthread_cond_init(&Q->write_ready, NULL);
    
    return err;  // obtained from the init functions (code omitted)
}

int enqueue(Queue *Q, char* item)
{
    pthread_mutex_lock(&Q->lock); // make sure no one else touches Q until we're done
    
    // while (Q->count == QUEUESIZE) 
	// {
    //     // wait for another thread to dequeue
    //     // pthread_cond_wait(&Q->write_ready, &Q->lock);
    //     // release lock & wait for a thread to signal write_ready
    // }
    // at this point, we hold the lock & Q->count < QUEUESIZE

    QNode *temp = malloc(sizeof(QNode));
    temp->data = malloc((strlen(item)+1) * sizeof(char));
    strcpy(temp->data, item);
    temp->next = NULL;

    if (Q->count == 0)
    {
        Q->head = temp;
        Q->rear = Q->head;
    }
    else {
        Q->rear->next = temp;
        Q->rear = temp;
    }
    Q->count++;
    
    pthread_mutex_unlock(&Q->lock); // now we're done
    pthread_cond_signal(&Q->read_ready); // wake up a thread waiting to read (if any)

    return 0;
}

char* dir_dequeue(Queue *Q)
{
    pthread_mutex_lock(&Q->lock);
    
    // while (Q->count == 0) 
	// {
    //     pthread_cond_wait(&Q->read_ready, &Q->lock);
    // }
	printf("\nCount: %d ActiveThreads: %d\n", Q->count, Q->activeThreads);
	if (Q->count == 0) {
		Q->activeThreads--;
		if (Q->activeThreads == 0) {
    		pthread_mutex_unlock(&Q->lock);
			pthread_cond_broadcast(&Q->read_ready);
			return NULL;
		}
		while (Q->count == 0 && Q->activeThreads > 0) {
			pthread_cond_wait(&Q->read_ready, &Q->lock);
		}
		if (Q->count == 0) {
			pthread_mutex_unlock(&Q->lock);
			return NULL;
		}
		Q->activeThreads++;
	}
	
    // now we have exclusive access and Queue is non-empty
    
    QNode *temp = Q->head;
	int size = strlen(temp->data);
	char* item = malloc((size+1)*sizeof(char));
	strcpy(item, temp->data);
    Q->head = Q->head->next;
    free(temp->data);
    free(temp);
    Q->count--;

    pthread_mutex_unlock(&Q->lock);
    
    return item;
    // return pthread_cond_signal(&Q->write_ready);
}

char* file_dequeue(Queue *Q)
{
    pthread_mutex_lock(&Q->lock);
    
    while (Q->count == 0) 
	{
        pthread_cond_wait(&Q->read_ready, &Q->lock);
    }
    // now we have exclusive access and Queue is non-empty
    
    QNode *temp = Q->head;
	int size = strlen(temp->data);
	char* item = malloc((size+1)*sizeof(char));
	strcpy(item, temp->data);
    Q->head = Q->head->next;
    free(temp->data);
    free(temp);
    Q->count--;

    pthread_mutex_unlock(&Q->lock);
    
    return item;
    // return pthread_cond_signal(&Q->write_ready);
}

void queue_print(Queue *Q) 
{
	printf("\n[Count: %d] ", Q->count);

	QNode *curr = Q->head;
   	while (curr != NULL) 
	{
        printf("%s -> ", curr->data);
        curr = curr->next;
    }
	printf("\n");
}

int directory_threads = 1;
int file_threads = 1;
int analysis_threads = 1;
char* file_name_suffix = ".txt";

int valid_suffix(char* file_path)
{
	int suffix_length = strlen(file_name_suffix);
	int file_path_index = strlen(file_path) - 1;

	if (suffix_length == 0) {
		for (int i = 0; i < strlen(file_path); i++) {
			if (file_path[i] == '.') {
				return 0;
			}
		}
	}

	//file_path must have at least one character and then a . before suffix
	if (file_path_index - 1 < suffix_length)
	{
		return 0;
	}
	int suffix_index = suffix_length - 1;
	while(suffix_index >= 0) 
	{
		if (file_path[file_path_index] != file_name_suffix[suffix_index])
		{
			return 0;
		}
		suffix_index--;
		file_path_index--;
	}
	return 1;
}

int is_directory(char *name)
{
	struct stat data;
	int err = stat(name, &data);

	if (err == -1)
	{
		perror("Error");
		exit(EXIT_FAILURE);
	}
	if (S_ISDIR(data.st_mode))
	{
		// S_ISDIR macro is true if the st_mode says the file is a directory
		// S_ISREG macro is true if the st_mode says the file is a regular file
		return 1;
	}
	return 0;
}

void print_optional_arguments()
{
	printf("Directory Threads: %d\n", directory_threads);
	printf("File Threads: %d\n", file_threads);
	printf("Analysis Threads: %d\n", analysis_threads);
	printf("File Name Suffix: %s\n", file_name_suffix);
}

int num_threads(char* arg)
{
	int threads;
	int arg_length = strlen(arg) - 1;
	char *option = malloc(sizeof(char) * arg_length);
	for (int i = 0; i < arg_length; i++)
	{
		option[i] = arg[i+2];
	}
	threads = atoi(option);
    free(option);
	return threads;
}

int is_argument(char* str)
{
	char tag = str[0];
	if (tag == '-')
	{
		if (strlen(str) < 3)
		{
			if (str[1] == 's')
			{
				file_name_suffix = "";
				return 1;
			}
			// invalid argument
			return 2;
		}
		char parameter = tolower(str[1]);
		if (parameter == 'd')
		{
			// directory threads
			directory_threads = num_threads(str);
		}
		else if (parameter == 'f')
		{
			// file threads
			file_threads = num_threads(str);
		}
		else if (parameter == 'a')
		{
			// analysis threads
			analysis_threads = num_threads(str);
		}
		else if (parameter == 's')
		{
			// file name suffix
			if (str[2] == '.' && strlen(str) > 3)
			{
				int suffix_length = strlen(str) - 1;
				char *suffix = malloc(sizeof(char) * suffix_length);
				for (int i = 0; i < suffix_length; i++) 
				{
					suffix[i] = str[i+2];
				}
				file_name_suffix = suffix;
			}
			else
			{
				// invalid argument
				return 2;
			}
		}
		else
		{
			// invalid argument
			return 2;
		}
		return 1;
	}
	return 0;
}

int process_arguments(int argc, char **argv, Queue *dirQueue, Queue *fileQueue)
{
	for (int i = 1; i < argc; i++)
	{
		char *curr_arg = argv[i];
		if (is_argument(curr_arg) == 0)
		{
			if (is_directory(curr_arg) == 1)
			{
				enqueue(dirQueue, curr_arg);
				printf("D: %s\n", curr_arg);
			}
			else
			{
				enqueue(fileQueue, curr_arg);
				printf("F: %s\n", curr_arg);
			}
		}
	}
	return 0;
}

typedef struct targs {
	Queue *dirQ;
	Queue *fileQ;
}targs;

void *dirThread(void *A) 
{
	targs *args = A;

	//while directory queue is not empty
	while (args->dirQ->count != 0) 
	{
		// read directory name from queue
		// open directory
		//     add entries to file or directory queues
		// repeat until directory queue is empty and all directory threads are waiting

		char* dir = dir_dequeue(args->dirQ);

		DIR* directory_p = opendir(dir);
		struct dirent* directory_entry_p;
		while ((directory_entry_p = readdir(directory_p))) 
		{
			char* de = directory_entry_p->d_name;
			char dir_de[100] = "";
			snprintf(dir_de, sizeof(dir_de), "%s/%s", dir, de);
			if (!((strcmp(de, ".") == 0) || (strcmp(de, "..") == 0))) {
				if (is_directory(dir_de) == 1) {
					enqueue(args->dirQ, dir_de);
					// queue_print(args->dirQ);
					// queue_print(args->fileQ);
				}
				else if (is_directory(dir_de) == 0) {
					if (valid_suffix(de) == 1) {
						// printf("\n%s\n", de);
						enqueue(args->fileQ, dir_de);
						// queue_print(args->fileQ);
					}
				}
			}
		}
		free(dir);
		free(directory_p);
	}
	return NULL;	
}

void *fileThread(void *A) {
	// file thread
    // loop
    //     read file name from file queue
    //     open file
    //     count all words in file
    //     compute WFD
    //     add to WFD repository
    // repeat until the queue is empty and the directory threads have stopped
		
	targs *args = A;

	while (args->fileQ->count != 0) {
		front = NULL;
		numWords = 0;

		char* name = file_dequeue(args->fileQ);
		int size = strlen(name);
		char* fileName = malloc((size+1)*sizeof(char));
		strcpy(fileName, name);
		free(name);
		int fd = open(fileName, O_RDONLY);
		getWords(fd);
    	getFrequencies(front);

		repoNode *r_Node = malloc(sizeof(repoNode));
		if (repoHead == NULL) {
			repoHead = r_Node;
		}
		else {
			repoNode *ptr = repoHead;
			while (ptr->next != NULL) {
				ptr = ptr->next;
			}
			ptr->next = r_Node;
		}
		r_Node->filename = fileName;
		r_Node->numWords = numWords;
		r_Node->WFD = front;
		r_Node->next = NULL;
	}
	return NULL;
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
	
	Queue dirQueue;
    Queue fileQueue;
    queue_init(&dirQueue);
	queue_init(&fileQueue);

	process_arguments(argc, argv, &dirQueue, &fileQueue);
	print_optional_arguments();

	int totalThreads = directory_threads + file_threads;
	pthread_t *tid = malloc(totalThreads * sizeof(pthread_t));
	targs *args = malloc(totalThreads * sizeof(targs));

	int i = 0;
	for (i = 0; i < directory_threads; i++) 
	{
		args[i].dirQ = &dirQueue;
		args[i].fileQ = &fileQueue;
		args[i].dirQ->activeThreads = directory_threads;
		// args[i].dirQ->activeThreads = 0;
		pthread_create(&tid[i], NULL, dirThread, &args[i]);
	}


	// sleep(1);


	for (; i < totalThreads; i++) 
	{
		args[i].fileQ = &fileQueue;
		pthread_create(&tid[i], NULL, fileThread, &args[i]);
	}


	// sleep(1);
	
	
	for (i = 0; i < totalThreads; i++) 
	{
		pthread_join(tid[i], NULL);
	}



	// queue_print(&dirQueue);
	// queue_print(&fileQueue);
	
	printList(repoHead);
	free(tid);
	free(args);
	cleanUp(repoHead);
	return 0;
}