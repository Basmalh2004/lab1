//commands.c
#define _POSIX_C_SOURCE 200809L
#include "commands.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>





char * built_iin_commands[] = {"showpid","pwd","cd","jobs","kill",
                               "fg","bg","quit","diff"};

//example function for printing errors from internal commands
void perrorSmash(const char* cmd, const char* msg)
{
    fprintf(stderr, "smash error:%s%s%s\n",
            cmd ? cmd : "",
            cmd ? ": " : "",
            msg);
}

//Job List Init
JobsList* Init_Jobs() {
    JobsList* j_list = (JobsList*)malloc(sizeof(JobsList));
    if(j_list == NULL) {
        return NULL;
    }
    for (int i = 0; i < JOBS_NUM_MAX; ++i) {
        j_list->jobs_list[i] = NULL;
    }
    return j_list;
}

//create command ---> Parse

//add command to list + set id


//example function for parsing commands
Command* ParseCmd(char* line)
{
    Command * cmd_ptr = (Command*)malloc(sizeof(Command));
    if (!cmd_ptr) {
        return NULL;
    }
    strcpy(cmd_ptr->full_cmd, line);
    cmd_ptr->full_cmd[strlen(cmd_ptr->full_cmd)-1] = '\0';
    cmd_ptr->start_time = 0;
    cmd_ptr->id = INIT_ID;
    cmd_ptr->pid = getpid();
    cmd_ptr->cmd_status = FOREGROUND;
    cmd_ptr->oldpwd = NULL;

    char* delimiters = " \t\n"; //parsing should be done by spaces, tabs or newlines
    char* cmd = strtok(line, delimiters); //read strtok documentation - parses string by delimiters

    if(!cmd) {
        free(cmd_ptr);
        return NULL; //this means no tokens were found, most like since command is invalid
    }

    strcpy(cmd_ptr->cmd, cmd);

    int nargs = 0;
    cmd_ptr->args[0] = NULL; //args don't include the command itself

    for(int i = 1; i < MAX_ARGS; i++)
    {
        char* arg = strtok(NULL, delimiters); //first arg NULL -> keep tokenizing from previous call
        if(!arg)
            break;

        if (strcmp(arg, "&") == 0) {
            cmd_ptr->cmd_status = BACKGROUND;
            break;
        }

        cmd_ptr->args[nargs] = strdup(arg);
        nargs++;
    }
    cmd_ptr->args[nargs] = NULL;
    cmd_ptr->num_of_args = nargs;

    return cmd_ptr;
    /*
    At this point cmd contains the command string and the args array contains
    the arguments. You can return them via struct/class, for example in C:
        typedef struct {
            char* cmd;
            char* args[MAX_ARGS];
        } Command;
    Or maybe something more like this:
        typedef struct {
            bool bg;
            char** args;
            int nargs;
        } CmdArgs;
    */
}

void AddCommand(JobsList* cmd_list, Command* command) {
    for(int i = 0; i < JOBS_NUM_MAX; ++i) {
        if( cmd_list->jobs_list[i] == NULL){ // TODO: || (cmd_list->jobs_list[i] != NULL && ) {
            command->id = i;
            cmd_list->jobs_list[i] = command;
            break;
        }
    }
}

int check_built_in(Command* command) {
    for(int i = 0; i < BUILT_IN_COMMANDS_MAX; ++i) {
        if (strcmp(command->cmd, built_iin_commands[i]) == 0) {
            return 1;
        };
    }
    return 0;
}

void CheckStatus(JobsList* jobs_list, Command* command) {
    if (check_built_in(command)) {
        if (command->cmd_status == BACKGROUND) {
            ExecBackgroundCmd(jobs_list,command);
        } else {
            CheckCommand(jobs_list, command);
        }
    } else {

    }

}

void CheckCommand(JobsList* jobs_list, Command* command) {
    if (strcmp(command->cmd, "showpid") == 0) {
        exec_showpid(jobs_list,command);
    } else if (strcmp(command->cmd, "pwd") == 0 ) {
        exec_pwd(jobs_list,command);
    } else if (strcmp(command->cmd, "cd") == 0 ) {
        exec_cd(jobs_list,command);
    } else if (strcmp(command->cmd, "jobs") == 0 ) {
        exec_jobs(jobs_list,command);
    } else if (strcmp(command->cmd, "kill") == 0 ) {
        exec_kill(jobs_list,command);
    } else if (strcmp(command->cmd, "fg") == 0 ) {
        exec_fg(jobs_list,command);
    } else if (strcmp(command->cmd, "bg") == 0 ) {
        exec_bg(jobs_list,command);
    } else if (strcmp(command->cmd, "quit") == 0 ) {
        exec_quit(jobs_list,command);
    } else if (strcmp(command->cmd, "diff") == 0 ) {
        exec_diff(jobs_list,command);
    }
}


void ExecBackgroundCmd(JobsList* jobs_list, Command* command) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0) {
        // Child process - execute the command
        // Note: This is a stub - actual implementation would use execvp
        exit(0);
    } else {
        // Parent process - store the background job
        command->pid = pid;
        command->start_time = time(NULL);
        printf("[%d] %d\n", command->id, pid);
    }
}

void exec_showpid(JobsList* cmd_list, Command* cmd) {

    if (!cmd) {
        fprintf(stderr, "smash error: internal: cmd is NULL in showpid\n");
        return;
    }

    if (cmd->num_of_args > 0) {
        fprintf(stderr, "smash error: showpid: expected 0 arguments\n");
        return;
    }

    cmd-> pid = getpid();
    printf("smash pid is %d\n", cmd->pid);
}

void exec_pwd(JobsList* cmd_list, Command* cmd){
    char* cwd = getcwd(NULL,0);
    if (!cmd) {
        fprintf(stderr, "smash error: internal: cmd is NULL in pwd\n");
        return;
    }

    if (cmd->num_of_args > 0) {
        fprintf(stderr, "smash error: pwd: expected 0 arguments\n");
        return;
    }
    if(!cwd) {
        perror("smash error: getcwd failed");
        return;
    }
    printf("%s \n",cwd);
}
int exec_cd(JobsList* cmd_list, Command* cmd) {
    extern char *oldpwd;

    if (!cmd) {
        fprintf(stderr, "smash error: internal: cmd is NULL in cd\n");
        return 1;
    }
    if (cmd->num_of_args > 1) {
        fprintf(stderr, "smash error: cd: too many arguments\n");
        return 1;
    }
    char *prev_cwd = getcwd(NULL, 0);
    if (!prev_cwd) {
        perror("smash error: getcwd failed");
        return 1;
    }
    char *targetdir = NULL;
    if (cmd->num_of_args == 0) {
        targetdir = getenv("HOME");
        if (!targetdir) {
            fprintf(stderr, "smash error: cd: HOME not set\n");
            free(prev_cwd);
            return 1;
        }
    } else {
        char *arg = cmd->args[0];

        if (strcmp(arg, "-") == 0) {
            if (!oldpwd) {
                fprintf(stderr, "smash error: cd: OLDPWD not set\n");
                free(prev_cwd);
                return 1;
            }
            targetdir = oldpwd;
        } else {
            targetdir = arg;
        }
    }

    if (chdir(targetdir) == -1) {
        perror("smash error: chdir failed");
        free(prev_cwd);
        return 1;
    }
    free(oldpwd);
    oldpwd = prev_cwd;

    return 0;
}

void exec_jobs(JobsList* cmd_list, Command* cmd) {
    if (!cmd) {
        fprintf(stderr, "smash error: internal: cmd is NULL in jobs\n");
        return;
    }
    if (cmd->num_of_args > 0) {
        fprintf(stderr, "smash error: jobs: expected 0 arguments\n");
        return;
    }

    for (int i = 0; i < JOBS_NUM_MAX; i++) {
        Command* job = cmd_list->jobs_list[i];
        if (job) {
            time_t elapsed = time(NULL) - job->start_time;
            const char* status = (job->cmd_status == STOPPED) ? "Stopped" : "Running";
            printf("[%d] %s %s %ld secs\n", job->id, job->full_cmd, status, (long)elapsed);
        }
    }
}

void exec_kill(JobsList* cmd_list, Command* cmd) {
    if (!cmd) {
        fprintf(stderr, "smash error: internal: cmd is NULL in kill\n");
        return;
    }
    if (cmd->num_of_args != 2) {
        fprintf(stderr, "smash error: kill: invalid arguments\n");
        return;
    }

    // First argument should be signal number (with optional -)
    char* sig_str = cmd->args[0];
    if (sig_str[0] != '-') {
        fprintf(stderr, "smash error: kill: invalid arguments\n");
        return;
    }

    int signum = atoi(sig_str + 1); // Skip the '-'
    if (signum <= 0) {
        fprintf(stderr, "smash error: kill: invalid signal number\n");
        return;
    }

    // Second argument should be job ID
    char* job_str = cmd->args[1];
    for (int i = 0; job_str[i]; i++) {
        if (!isdigit(job_str[i])) {
            fprintf(stderr, "smash error: kill: invalid job id\n");
            return;
        }
    }

    int job_id = atoi(job_str);
    Command* job = findjobbyid(cmd_list, job_id);
    if (!job) {
        fprintf(stderr, "smash error: kill: job-id %d does not exist\n", job_id);
        return;
    }

    if (kill(job->pid, signum) == -1) {
        perror("smash error: kill failed");
        return;
    }

    printf("signal number %d was sent to pid %d\n", signum, job->pid);
}

Command *findjobbyid(JobsList* cmd_list, int id) {
    for (int i = 0; i < JOBS_NUM_MAX; ++i) {
        if (cmd_list->jobs_list[i] && cmd_list->jobs_list[i]->id == id) {
            return cmd_list->jobs_list[i];
        }
    }
    return NULL;
}

Command *findmaxjobid(JobsList* cmd_list) {
    Command* max = NULL;
    for (int i = 0; i < JOBS_NUM_MAX; ++i) {
        if (!cmd_list->jobs_list[i]) {
            continue;
        }
        if (!max || cmd_list->jobs_list[i]->id > max->id) {
            max = cmd_list->jobs_list[i];
        }
    }
    return max;
}

void removejobbyid(JobsList* cmd_list, int job_id) {
    if (!cmd_list) {
        return;
    }
    for (int i = 0; i < JOBS_NUM_MAX; ++i) {
        Command *removeable = cmd_list->jobs_list[i];
        if (removeable && removeable->id == job_id) {
            cmd_list->jobs_list[i] = NULL;
            for (int j = 0; j < removeable->num_of_args; j++) {
                free(removeable->args[j]);
            }
            free(removeable->oldpwd);
            free(removeable);
            return;
        }
    }
}
int exec_fg(JobsList* cmd_list, Command* cmd) {
    if (!cmd) {
        fprintf(stderr, "smash error: fg: invalid command\n");
        return 1;
    }
    if (cmd->num_of_args > 1) {
        fprintf(stderr, "smash error: fg: too many arguments\n");
        return 1;
    }

    Command *job = NULL;
    if (cmd->num_of_args == 1) {
        // Check if argument is a valid number
        char* arg = cmd->args[0];
        for (int i = 0; arg[i]; i++) {
            if (!isdigit(arg[i])) {
                fprintf(stderr, "smash error: fg: invalid job id\n");
                return 1;
            }
        }
        int job_id = atoi(arg);
        job = findjobbyid(cmd_list, job_id);
        if (!job) {
            fprintf(stderr, "smash error: fg: job-id %d does not exist\n", job_id);
            return 1;
        }
    } else if (cmd->num_of_args == 0) {
        job = findmaxjobid(cmd_list);
        if (!job) {
            fprintf(stderr, "smash error: fg: jobs list is empty\n");
            return 1;
        }
    }

    printf("%s\n", job->full_cmd);

    // If job is stopped, send SIGCONT to resume it
    if (job->cmd_status == STOPPED) {
        if (kill(job->pid, SIGCONT) == -1) {
            perror("smash error: kill failed");
            return 1;
        }
    }

    // Wait for the job to finish or stop
    int status;
    pid_t ret = waitpid(job->pid, &status, WUNTRACED);
    if (ret == -1) {
        perror("smash error: waitpid failed");
        return 1;
    }

    if (WIFSTOPPED(status)) {
        // Job was stopped (Ctrl-Z)
        job->cmd_status = STOPPED;
    } else {
        // Job finished, remove it from the jobs list
        removejobbyid(cmd_list, job->id);
    }

    return 0;
}

void exec_bg(JobsList* cmd_list, Command* cmd) {
    if (!cmd) {
        fprintf(stderr, "smash error: bg: invalid command\n");
        return;
    }
    if (cmd->num_of_args > 1) {
        fprintf(stderr, "smash error: bg: too many arguments\n");
        return;
    }

    Command *job = NULL;
    if (cmd->num_of_args == 1) {
        // Check if argument is a valid number
        char* arg = cmd->args[0];
        for (int i = 0; arg[i]; i++) {
            if (!isdigit(arg[i])) {
                fprintf(stderr, "smash error: bg: invalid job id\n");
                return;
            }
        }
        int job_id = atoi(arg);
        job = findjobbyid(cmd_list, job_id);
        if (!job) {
            fprintf(stderr, "smash error: bg: job-id %d does not exist\n", job_id);
            return;
        }
    } else if (cmd->num_of_args == 0) {
        job = findmaxjobid(cmd_list);
        if (!job) {
            fprintf(stderr, "smash error: bg: jobs list is empty\n");
            return;
        }
    }

    if (job->cmd_status != STOPPED) {
        fprintf(stderr, "smash error: bg: job-id %d is already running in the background\n", job->id);
        return;
    }

    printf("%s\n", job->full_cmd);

    // Send SIGCONT to resume the stopped job
    if (kill(job->pid, SIGCONT) == -1) {
        perror("smash error: kill failed");
        return;
    }

    job->cmd_status = BACKGROUND;
}

void exec_quit(JobsList* cmd_list, Command* cmd) {
    if (!cmd) {
        exit(0);
    }

    if (cmd->num_of_args > 1) {
        fprintf(stderr, "smash error: quit: too many arguments\n");
        return;
    }

    if (cmd->num_of_args == 1 && strcmp(cmd->args[0], "kill") == 0) {
        // Kill all jobs
        for (int i = 0; i < JOBS_NUM_MAX; i++) {
            Command* job = cmd_list->jobs_list[i];
            if (job) {
                printf("Killing job %d: %s\n", job->id, job->full_cmd);
                if (kill(job->pid, SIGKILL) == -1) {
                    perror("smash error: kill failed");
                }
            }
        }
    }

    exit(0);
}

void exec_diff(JobsList* cmd_list, Command* cmd) {
    if (!cmd) {
        fprintf(stderr, "smash error: diff: invalid command\n");
        return;
    }
    if (cmd->num_of_args != 2) {
        fprintf(stderr, "smash error: diff: invalid arguments\n");
        return;
    }

    char* file1 = cmd->args[0];
    char* file2 = cmd->args[1];

    FILE* f1 = fopen(file1, "r");
    if (!f1) {
        perror("smash error: diff");
        return;
    }

    FILE* f2 = fopen(file2, "r");
    if (!f2) {
        perror("smash error: diff");
        fclose(f1);
        return;
    }

    // Read both files and compare
    char line1[CMD_LENGTH_MAX];
    char line2[CMD_LENGTH_MAX];
    int line_num = 1;
    int differ = 0;

    while (1) {
        char* r1 = fgets(line1, CMD_LENGTH_MAX, f1);
        char* r2 = fgets(line2, CMD_LENGTH_MAX, f2);

        if (r1 == NULL && r2 == NULL) {
            // Both files ended at the same time
            break;
        }

        if (r1 == NULL || r2 == NULL || strcmp(line1, line2) != 0) {
            differ = 1;
            break;
        }

        line_num++;
    }

    fclose(f1);
    fclose(f2);

    if (differ) {
        printf("diff: %s %s differ\n", file1, file2);
    } else {
        printf("diff: %s %s are identical\n", file1, file2);
    }
}


