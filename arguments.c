#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
/*
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>
*/

int directory_threads = 1;
int file_threads = 1;
int analysis_threads = 1;
char* file_name_suffix = ".txt";

int valid_suffix(char* file_path)
{
	int suffix_length = strlen(file_name_suffix);
	int file_path_index = strlen(file_path) - 1;
	//file_path must have at least one character and then a . before suffix
	if (file_path_index - 1 < suffix_length)
	{
		return 0;
	}
	int suffix_index = suffix_length - 1;
	while(suffix_index >= 0) {
		if (file_path[file_path_index] != file_name_suffix[suffix_index])
		{
			return 0;
		}
	}
	return 1;
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
	if (threads < 1)
	{
		perror("Error: Invalid Optional Argument");
		exit(EXIT_FAILURE);
	}
	return threads;
}

int is_argument(char* str)
{
	char tag = str[0];
	if (tag == '-')
	{
		if (strlen(str) < 3)
		{
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
			if (str[2] == '.')
			{
				int suffix_length = strlen(str) - 1;
				char *suffix = malloc(sizeof(char) * suffix_length);
				for (int i = 0; i < suffix_length; i++) {
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

int process_arguments(int argc, char **argv)
{
	for (int i = 1; i < argc; i++)
	{
		char *curr_arg = argv[i];
		int argument_check = is_argument(curr_arg);
		if (argument_check == 0)
		{
			if (is_directory(curr_arg) == 1)
			{
				printf("D: %s\n", curr_arg);
			}
			else
			{
				printf("F: %s\n", curr_arg);
			}
		}
		else if (argument_check == 2)
		{
			perror("Error: Invalid Optional Argument");
			exit(EXIT_FAILURE);
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
	process_arguments(argc, argv);
	print_optional_arguments();
}