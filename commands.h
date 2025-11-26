#ifndef COMMANDS_H
#define COMMANDS_H
/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#define CMD_LENGTH_MAX 120
#define MAX_ARGS 20
#define JOBS_NUM_MAX 100
#define BUILT_IN_COMMANDS_MAX 9
#define INIT_ID -1

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
    //feel free to add more values here or delete this
} ParsingError;

typedef enum {
    SMASH_SUCCESS = 0,
    SMASH_QUIT,
    SMASH_FAIL
    //feel free to add more values here or delete this
} CommandResult;

typedef enum {
    BACKGROUND,
    FOREGROUND,
    STOPPED
} CommandStatus;

typedef struct {
    char* cmd;
    char* full_cmd;
    char *oldpwd;
    char* args[MAX_ARGS];
    int id;
    int pid;
    time_t start_time;
    int num_of_args;
    CommandStatus cmd_status;
} Command;

typedef struct {
    Command* jobs_list[JOBS_NUM_MAX];
} JobsList;


/*=============================================================================
* global functions
=============================================================================*/
Command* ParseCmd(char* line);
void AddCommand(JobsList* cmd_list, Command* cmd);
void ExecuteCommand(JobsList* jobs_list, Command* cmd);
void ExecBackgroundCmd(JobsList* jobs_list, Command* command);
void CheckStatus(JobsList* jobs_list, Command* command);
void CheckCommand(JobsList* jobs_list, Command* cmd);
JobsList* Init_Jobs();

void exec_showpid(JobsList* cmd_list, Command* cmd);
void exec_pwd(JobsList* cmd_list, Command* cmd);
void exec_cd(JobsList* cmd_list, Command* cmd);
void exec_jobs(JobsList* cmd_list, Command* cmd);
void exec_kill(JobsList* cmd_list, Command* cmd);
void exec_fg(JobsList* cmd_list, Command* cmd);
void exec_bg(JobsList* cmd_list, Command* cmd);
void exec_quit(JobsList* cmd_list, Command* cmd);
void exec_diff(JobsList* cmd_list, Command* cmd);


#endif //COMMANDS_H