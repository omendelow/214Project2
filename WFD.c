#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

typedef struct Node{
  char *word;
  int count;
  double frequency;
  struct Node *next;
}Node;

typedef struct strbuf_t{
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

void printList(Node *front) {
    Node *curr = front;
   	while (curr != NULL) {
        printf("%s, %d, %f-> ", curr->word, curr->count, curr->frequency);
        curr = curr->next;
    }
}

void cleanUp(Node *front) {
    Node* curr = front;
    while (curr != NULL) {
        Node* temp = curr;
        curr = curr->next;
        free(temp->word);
        free(temp);
    }
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Not enough arguments\n");
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("Error: File does not exist.");
		exit(EXIT_FAILURE);
	}

    getWords(fd);
    getFrequencies(front);
    printList(front);
    cleanUp(front);
    // printf("%d\n", numWords);
	

}