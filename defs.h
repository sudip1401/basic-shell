#ifndef	DEFS_H
#define DEFS_H

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

// Input Modes
#define I_NONE	0
#define I_FILE  1
#define I_PIPE  2

// Output Modes
#define O_NONE		0
#define O_WRITE		1
#define O_APPEND	2
#define O_PIPE		3


// Run Next Command Modes
#define NEXT_ON_ANY        1
#define NEXT_ON_SUCCESS    2
#define NEXT_ON_FAIL       3

extern int verbose;

#define LOG(format, ...) do { 					\
	if(verbose)									\
		fprintf(stdout, format, ##__VA_ARGS__); \
} while(0);

#define STR(X)	#X

typedef struct ArgX {
        char *arg;
        struct ArgX *next;
} Arg;

typedef struct CommandX {
        char * cmd;
        char **argv;

        Arg  *arg_list;
        Arg  *last_arg;

        char *input_file;
        int  input_mode;
        int  input_fd;

        char *output_file;
        int  output_mode;
        int  output_fd;

        int  next_command_exec_on;
        pid_t pid;
        struct CommandX *next;
} Command;

#endif
