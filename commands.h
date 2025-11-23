#ifndef COMMANDS_H
#define COMMANDS_H

// Must define POSIX macros BEFORE any includes
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include "my_system_call.h"
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "signals.h"

#define CMD_LENGTH_MAX 120
#define ARGS_NUM_MAX 20
#define MAX_ARGS ARGS_NUM_MAX
#define JOBS_NUM_MAX 100

typedef enum {
    FOREGROUND = 0,
    BACKGROUND ,
    STOPPED 
} JobStatus;

typedef struct {
			char* cmd;
			char* args[ARGS_NUM_MAX];
            int nargs;
            int isBackground;  // 1 if command ends with &, 0 otherwise
		} Command;

typedef struct {
	int jobId;
	pid_t jobPid;
    char jobCmd_str[CMD_LENGTH_MAX];
	Command jobCmd;
	JobStatus status;
    time_t startTime;
} Job;
// for the cd i save it ok ?
extern Job jobs_list[JOBS_NUM_MAX];
extern char old_pwd[CMD_LENGTH_MAX];

/*=============================================================================
* error handling - some useful macros and examples of error handling,
* feel free to not use any of this
=============================================================================*/
#define ERROR_EXIT(msg) \
    do { \
        fprintf(stderr, "%s: %d\n%s", __FILE__, __LINE__, msg); \
        exit(1); \
    } while(0);

static inline void* _validatedMalloc(size_t size)
{
    void* ptr = malloc(size);
    if(!ptr) ERROR_EXIT("malloc");
    return ptr;
}

// example usage:
// char* bufffer = MALLOC_VALIDATED(char, MAX_LINE_SIZE);
// which automatically includes error handling
#define MALLOC_VALIDATED(type, size) \
    ((type*)_validatedMalloc((size)))


/*=============================================================================
* error definitions
=============================================================================*/
typedef enum  {
	INVALID_COMMAND = 0,
    VALID_COMMAND = 1 
	//feel free to add more values here or delete this
} ParsingError;

typedef enum {
	SMASH_SUCCESS = 0,
	SMASH_QUIT,
	SMASH_FAIL , 
    STILL_STOPPED
	//feel free to add more values here or delete this
} CommandResult;

/*=============================================================================
* global functions
=============================================================================*/

// Parsing
int parseCmdExample(char* line, Command* cmd_t);

// Helper functions
void wait_or_kill(pid_t pid);
void buildCommandString(Command* cmd, char* result, int max_size);
void perrorSmash(const char* cmd, const char* msg);

// Jobs management
void jobes_list_print();
void cleanup_finished_jobs();  // Remove finished jobs from list

// Built-in commands
int kill_signal(int signum, int jobId);
int fg_command(int jobId);
int bg_command(int jobId);
int quit_command(int kill_jobs);
int diff_cmd(char* path1, char* path2);
int executeExternal(Command* cmd);
// dont do it please
// Command handler
int command_handler(Command* cmd);

#endif //COMMANDS_H