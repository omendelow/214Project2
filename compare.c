#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <fcntl.h> 
#include <assert.h>
*/

#ifndef DEBUG
#define DEBUG 0
#endif

//max filepath length
int fp_length = 100;

typedef struct {
	size_t length;
	size_t used;
	char** paths;
} arraylist_t;

int al_init(arraylist_t *L, size_t length) {
	L->paths = malloc(sizeof(char) * fp_length * length);
	if (!L->paths) return 1;

	L->length = length;
	L->used = 0;

	return 0;
}

void al_destroy(arraylist_t *L) {
	free(L->paths);
}

int al_append(arraylist_t *L, char* fp) {
	if (L->used == L->length) {
		size_t size = L->length * 2;
		char **p = realloc(L->paths, sizeof(char) * fp_length * size);
		if (!p) return 1;

		L->paths = p;
		L->length = size;
		if (DEBUG) printf("Increased size to %lu\n", size);
	}

	L->paths[L->used] = fp;
	++L->used;

	return 0;
}

int al_remove(arraylist_t *L, char* fp) {
	if (L->used == 0) return 1;

	--L->used;

	if (fp) strcpy(fp, L->paths[L->used]);

	return 1;
}

void al_print(arraylist_t *L) {
	for (int i =0; i < L->length-1; i++) {
		printf("%s, ",L->paths[i]);
	}
	printf("%s\n", L->paths[L->length-1]);
}

int resize(arraylist_t *L, size_t size) {
	char **p = realloc(L->paths, sizeof(char) * fp_length * size);
	if (!p) return 1;

	L->paths = p;
	L->length = size;
	return 0;
}


int al_insert(arraylist_t *L, int index, char* fp) {
	if (index < 0) return 1;
	else if (index <= L->used) {
		if (L->used + 1 >= L->length) {
			size_t size = L->length * 2;
			int p = resize(L, size);
			if (p == 1) return 1;
		}
		for (int i = L->used+1; i > index; i--) {
			L->paths[i] = L->paths[i-1];
		}
		L->paths[index] = fp;
		++L->used;
	}
	else {
		if (index >= L->length) {
			size_t size= L->length * 2;
			if (index+1 > size) size = index+1;
			int p = resize(L, size);
			if (p == 1) return 1;
		}
		L->paths[index] = fp;
		L->used = index+1;
	}
	return 0;
}


int main(int argc, char **argv) {

}
