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
#include <math.h>

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

void cleanUpWFD(Node *head) {
    Node* curr = head;
	while (curr != NULL)
	{
		Node* temp = curr;
		curr = curr->next;
		free(temp->word);
		free(temp);
	}
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

int destroy(Queue *Q)
{
	pthread_mutex_destroy(&Q->lock);
	pthread_cond_destroy(&Q->read_ready);
	// pthread_cond_destroy(&Q->write_ready);

	return 0;
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
    
	pthread_cond_signal(&Q->read_ready); // wake up a thread waiting to read (if any)
    pthread_mutex_unlock(&Q->lock); // now we're done

    return 0;
}

char* dir_dequeue(Queue *Q)
{
    pthread_mutex_lock(&Q->lock);
    
	// printf("\nQueue Count: %d ActiveThreads: %d\n", Q->count, Q->activeThreads);
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
		if (file_path[0] != '.') {
			return 1;
		}
		else
		{
			return 0;
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

typedef struct zargs {
	unsigned comparisons;
	int thread_number;
	struct comp_result *results;
}zargs;

int get_queue_count(char* queuetype, Queue *Q)
{
	int count = 0;
	pthread_mutex_lock(&Q->lock);
	count = Q->count;
	pthread_mutex_unlock(&Q->lock);
	// printf("%s Queue count: %d\n", queuetype, count);
	return count;
}

void *dirThread(void *A) 
{
	targs *args = A;

	//while directory queue is not empty
	while (get_queue_count("Directory", args->dirQ) != 0) 
	{
		// read directory name from queue
		// open directory
		//     add entries to file or directory queues
		// repeat until directory queue is empty and all directory threads are waiting

		// printf("what\n");
		pthread_mutex_lock(&args->fileQ->lock);

		char* dir = dir_dequeue(args->dirQ);

		DIR* directory_p = opendir(dir);
		struct dirent* directory_entry_p;
		while ((directory_entry_p = readdir(directory_p))) 
		{
			char* de = directory_entry_p->d_name;
			char sub[2] = "";
			strncpy(sub, de, 1);
			if (!(strcmp(sub, ".") == 0)) {
				char dir_de[100] = "";
				snprintf(dir_de, sizeof(dir_de), "%s/%s", dir, de);
				if (is_directory(dir_de) == 1) {
					// pthread_mutex_unlock(&args->dirQ->lock);
					enqueue(args->dirQ, dir_de);
					// queue_print(args->dirQ);
					// queue_print(args->fileQ);
				}
				else if (is_directory(dir_de) == 0) {
					if (valid_suffix(de) == 1) {
						// printf("no\n");
						pthread_mutex_unlock(&args->fileQ->lock);
						enqueue(args->fileQ, dir_de);
						// queue_print(args->fileQ);
					}
				}
			}
		}
		pthread_mutex_unlock(&args->fileQ->lock);

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

	while ((get_queue_count("File", args->fileQ) != 0) ) {
		// if (get_queue_count("File", args->fileQ) != 0)
		// {
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
		// }
	}
	return NULL;
}

struct comp_result
{
	repoNode *f1, *f2;
	char *file1, *file2;
	unsigned tokens; // word count of file 1 + file 2
	double distance; // JSD between file 1 and file 2
	// pthread_mutex_t lock;
};

double get_kld(repoNode *repo, Node* node)
{
	//printf("FILE: %s\n", repo->filename);
	double sum = 0;
	Node* curr_fp = repo->WFD;
	Node* curr_node = node;
	while (curr_fp != NULL)
	{
		//printf("%s vs %s\n", curr_fp->word, curr_node->word);
		if (curr_node == NULL) break;
		int compare = strcmp(curr_fp->word, curr_node->word);
		if (compare == 0)
		{
			// printf("%f * log2(%f / %f) = %f * %f = %f\n", curr_fp->frequency, curr_fp->frequency, curr_node->frequency, curr_fp->frequency, (log(curr_fp->frequency/curr_node->frequency)/log(2)), (curr_fp->frequency * (log(curr_fp->frequency/curr_node->frequency)/log(2))));
			sum += curr_fp->frequency * (log(curr_fp->frequency/curr_node->frequency)/log(2));
			curr_fp = curr_fp->next;
			curr_node = curr_node->next;
		}
		else if (compare < 0)
		{
			curr_fp = curr_fp->next;
		}
		else
		{
			curr_node = curr_node->next;
		}
		//printf("kld- %f\n", sum);
	}
	//printf("sum- %f\n", sum);
	return sum;
}

double get_jsd(repoNode* one, repoNode* two, Node* avg)
{
	double kld_one = get_kld(one, avg);
	double kld_two = get_kld(two, avg);
	//printf("KLD ONE: %f\n", kld_one);
	//printf("KLD TWO: %f\n", kld_two);
	double jsd = sqrt(0.5 * (kld_one + kld_two));
	// printf("jsd- %f\n", jsd);
	return jsd;
}

void build_node(Node* node, char* word, double freq1, double freq2)
{
	int size = strlen(word);
	node->word = malloc((size+1)*sizeof(char));
	strcpy(node->word, word);
	// node->count = count1 + count2;
	node->frequency = (freq1+freq2) / 2;
	// printf("Avg. freq: %f\n", node->frequency);
}

double comparison_avg(repoNode *one, repoNode *two)
{
	// printf("comparison_avg starting\n");
	Node *head = malloc(sizeof(Node));
	head->next = NULL;
	Node *curr_node = head;
	Node *new = NULL;
	//printf("total_num_word: %d\n", total_num_word);
	Node* one_WFD = one->WFD;
	Node* two_WFD = two->WFD;
	while (one_WFD != NULL || two_WFD != NULL)
	{
		if (one_WFD == NULL)
		{
			build_node(curr_node, two_WFD->word, 0, two_WFD->frequency);
			two_WFD = two_WFD->next;
		}
		else if (two_WFD == NULL)
		{
			build_node(curr_node, one_WFD->word, one_WFD->frequency, 0);
			one_WFD = one_WFD->next;
		}
		else
		{
			int one_vs_two = strcmp(one_WFD->word, two_WFD->word);
			if (one_vs_two == 0)
			{
				build_node(curr_node, one_WFD->word, one_WFD->frequency, two_WFD->frequency);
				one_WFD = one_WFD->next;
				two_WFD = two_WFD->next;
			}
			else if (one_vs_two > 0)
			{
				build_node(curr_node, two_WFD->word, 0, two_WFD->frequency);
				two_WFD = two_WFD->next;
			}
			else
			{
				build_node(curr_node, one_WFD->word, one_WFD->frequency, 0);
				one_WFD = one_WFD->next;
			}
		}
		new = malloc(sizeof(Node));
		new->word = NULL;
		new->next = NULL;
		curr_node->next = new;
		curr_node = new;
	}
	double to_return =  get_jsd(one, two, head);
	cleanUpWFD(head);
	// printf("comparison_avg ending\n");
	return to_return;
}

void print_file_pairs(repoNode * head)
{
	int pair_counter = 0;
	repoNode *next_file;
	while (head != NULL)
	{
		next_file = head->next;
		while (next_file != NULL)
		{
			printf("\n%d: %s vs %s\n", pair_counter, head->filename, next_file->filename);
			comparison_avg(head, next_file);
			next_file = next_file->next;
			pair_counter++;
		}
		head = head->next;	
	}
}

double compute_jsd(repoNode *f1, repoNode *f2)
{
	double distance = 0;
	distance = comparison_avg(f1, f2);
	return distance;
}

static int sort_comps(const void *r1, const void *r2)
{
	int comp = ((struct comp_result*) r2)->tokens - ((struct comp_result*)r1)->tokens;
	return comp;
}

void print_result(struct comp_result *result)
{
	printf("%f %d %s %s\n", result->distance, result->tokens, result->file1, result->file2);
}

unsigned get_num_files(repoNode* head)
{
	unsigned counter = 0;	
	repoNode* start = head;
	while (start != NULL)
	{
		counter++;
		start = start->next;
	}
	return counter;
}

void *analysisThread(void *A) {

	zargs *args2 = A;

	int i = args2->thread_number;
	int comparisons = args2->comparisons;
	struct comp_result *results = args2->results;

	while (i < comparisons)
	{
		results[i].distance = compute_jsd(results[i].f1, results[i].f2);
		i += analysis_threads;
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

	int collection_threads = directory_threads + file_threads;
	int totalThreads = collection_threads + analysis_threads;
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

	for (; i < collection_threads; i++) 
	{
		args[i].fileQ = &fileQueue;
		pthread_create(&tid[i], NULL, fileThread, &args[i]);
	}
	
	for (i = 0; i < collection_threads; i++) 
	{
		pthread_join(tid[i], NULL);
	}

	// printList(repoHead);

	//updated JSD methods
	unsigned num_files = get_num_files(repoHead);
	unsigned comparisons = num_files * (num_files - 1) / 2;
	struct comp_result *results = malloc(comparisons * sizeof(struct comp_result));
	i = 0;
	repoNode *f1 = repoHead;
	repoNode *f2;
	while (f1 != NULL)
	{
		f2 = f1->next;
		while (f2 != NULL)
		{
			results[i].file1 = f1->filename;
			results[i].file2 = f2->filename;
			results[i].f1 = f1;
			results[i].f2 = f2;
			results[i].tokens = f1->numWords + f2->numWords;
			f2 = f2->next;
			i++;
		}
		f1 = f1->next;	
	}

	zargs *args2 = malloc(totalThreads * sizeof(zargs));

	for (i = 0; i < analysis_threads; i++)
	{
		args2[i].comparisons = comparisons;
		args2[i].thread_number = i;
		args2[i].results = results;
		pthread_create(&tid[i+collection_threads], NULL, analysisThread, &args2[i]);
	}

	for (i = collection_threads; i < totalThreads; i++)
	{
		pthread_join(tid[i], NULL);
	}

	qsort(results, comparisons, sizeof(struct comp_result), sort_comps);
	
	printf("\n");
	for (i = 0; i < comparisons; i++)
	{
		print_result(&results[i]);
	}

	destroy(&dirQueue);
	destroy(&fileQueue);
	free(args2);
	free(tid);
	free(args);
	free(results);
	cleanUp(repoHead);
	return 0;
}
