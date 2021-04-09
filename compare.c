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

#ifndef DEBUG
#define DEBUG 0
#endif

//max filepath length
int fp_length = 100;

typedef struct
{
	size_t length;
	size_t used;
	char **paths;
} arraylist_t;

int al_init(arraylist_t *L, size_t length)
{
	L->paths = malloc(sizeof(char) * fp_length * length);
	if (!L->paths)
		return 1;

	L->length = length;
	L->used = 0;

	return 0;
}

void al_destroy(arraylist_t *L)
{
	free(L->paths);
}

void al_print(arraylist_t *L)
{
	for (int i = 0; i < L->length; i++)
	{
		printf("%s%s", L->paths[i], (i == L->length - 1 ? "\n" : ", "));
	}
}

int al_append(arraylist_t *L, char *fp)
{
	if (L->used == L->length)
	{
		size_t size = L->length * 2;
		char **p = realloc(L->paths, sizeof(char) * fp_length * size);
		if (!p)
			return 1;

		L->paths = p;
		L->length = size;
		if (DEBUG)
			printf("Increased size to %lu\n", size);
	}
	char *word = malloc(sizeof(char) * strlen(fp));
	strcpy(word, fp);
	L->paths[L->used] = word;
	++L->used;
	return 0;
}

// int al_remove(arraylist_t *L, char *fp)
// {
// 	if (L->used == 0)
// 		return 1;

// 	--L->used;

// 	if (fp)
// 		strcpy(fp, L->paths[L->used]);

// 	return 1;
// }

int resize(arraylist_t *L, size_t size)
{
	char **p = realloc(L->paths, sizeof(char) * fp_length * size);
	if (!p)
		return 1;

	L->paths = p;
	L->length = size;
	return 0;
}

int al_insert(arraylist_t *L, int index, char *fp)
{
	if (index < 0)
		return 1;
	else if (index <= L->used)
	{
		if (L->used + 1 >= L->length)
		{
			size_t size = L->length * 2;
			int p = resize(L, size);
			if (p == 1)
				return 1;
		}
		for (int i = L->used + 1; i > index; i--)
		{
			L->paths[i] = L->paths[i - 1];
		}
		L->paths[index] = fp;
		++L->used;
	}
	else
	{
		if (index >= L->length)
		{
			size_t size = L->length * 2;
			if (index + 1 > size)
				size = index + 1;
			int p = resize(L, size);
			if (p == 1)
				return 1;
		}
		L->paths[index] = fp;
		L->used = index + 1;
	}
	return 0;
}

int is_directory(char *name)
{
	struct stat data;
	int err = stat(name, &data);
	// should confirm err == 0
	if (err)
	{
		perror("Error.");
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

void process_de(arraylist_t *L, char *dir_de)
{
	al_append(L, dir_de);
}

int process_directory(arraylist_t *L, char *dir_name)
{
	DIR *directory_p = opendir(dir_name);
	struct dirent *directory_entry_p;
	while ((directory_entry_p = readdir(directory_p)))
	{
		//puts directory_entry -> directory_name
		char *de = directory_entry_p->d_name;
		//check for . at beginning of filename
		char sub[2] = "";
		strncpy(sub, de, 1);
		if (!(strcmp(sub, ".") == 0))
		{
			char dir_de[100] = "";
			snprintf(dir_de, sizeof(dir_de), "%s/%s", dir_name, de);
			//check if subdirectory
			if (is_directory(dir_de) == 1)
			{
				process_directory(L, dir_de);
			}
			else
			{
				al_append(L, dir_de);
			}
		}
	}
	if (closedir(directory_p) == -1)
	{
		perror("Error: Unable to close directory.");
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}

/*
int process_file(char* file_name, unsigned page_width) {
	// page width and filename given
	int fd = open(file_name, O_RDONLY);
	if (fd < 0) {
		perror("Error: File does not exist.");
		exit(EXIT_FAILURE);
	}
	int error = wrap(page_width, fd, 1);
	if (error == 1) {
		close(fd);
		perror("Error: Page width exceeded.");
		exit(EXIT_FAILURE);
	}
	close(fd);
	return EXIT_SUCCESS;
}
*/
// int process_path(char *curr_path, arraylist_t *L)
// {

// }

int process_arguments(int argc, char **argv, arraylist_t *L)
{
	for (int i = 1; i < argc; i++)
	{
		char *curr_path = argv[i];
		// check if directory or file
		if (is_directory(curr_path) == 1)
		{
			process_directory(L, curr_path);
		}
		else
		{
			al_append(L, curr_path);
		}
	}

	return 0;
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
	arraylist_t L;
	al_init(&L, argc);
	process_arguments(argc, argv, &L);
	al_print(&L);
	al_destroy(&L);
}
