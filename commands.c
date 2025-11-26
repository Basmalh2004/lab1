//commands.c
#include "commands.h"

#include "my_system_call.h"
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>





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

    // Initialize fields
    cmd_ptr->oldpwd = NULL;
    cmd_ptr->cmd_status = FOREGROUND;
    cmd_ptr->start_time = 0;
    cmd_ptr->id = INIT_ID;
    cmd_ptr->pid = getpid();

    strcpy(cmd_ptr->full_cmd, line);
    cmd_ptr->full_cmd[strlen(cmd_ptr->full_cmd)-1] = '\0';

    char* delimiters = " \t\n"; //parsing should be done by spaces, tabs or newlines
    char* cmd = strtok(line, delimiters); //read strtok documentation - parses string by delimiters

    if(!cmd) {
        free(cmd_ptr);
        return NULL; //this means no tokens were found, most likely since command is invalid
    }

    strcpy(cmd_ptr->cmd, cmd);

    int nargs = 0;
    cmd_ptr->args[0] = NULL; //first arg is NULL by default

    for(int i = 0; i < MAX_ARGS; i++)
    {
        char* arg = (i == 0) ? cmd : strtok(NULL, delimiters);
        if(!arg)
            break;

        if (strcmp(arg, "&") == 0) {
            cmd_ptr->cmd_status = BACKGROUND;
            break;
        }

        // Copy argument - skip first one as it's the command itself
        if (i > 0) {
            cmd_ptr->args[nargs] = arg;
            nargs++;
        }
    }

    cmd_ptr->num_of_args = nargs;
    // NULL terminate args
    if (nargs < MAX_ARGS) {
        cmd_ptr->args[nargs] = NULL;
    }

    return cmd_ptr;
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
    pid_t pid = my_system_call(SYS_FORK);
    if (pid < 0) {
        //TODO: Print Error
    }
    if (pid == 0) {


    } else {

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
    if (!cmd) {
        fprintf(stderr, "smash error: internal: cmd is NULL in pwd\n");
        return;
    }

    if (cmd->num_of_args > 0) {
        fprintf(stderr, "smash error: pwd: expected 0 arguments\n");
        return;
    }

    char* cwd = getcwd(NULL,0);
    if(!cwd) {
        perror("smash error: getcwd failed");
        return;
    }
    printf("%s\n",cwd);
    free(cwd);
}
int exec_cd(JobsList* cmd_list, Command* cmd) {
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
            if (!cmd->oldpwd) {
                fprintf(stderr, "smash error: cd: OLDPWD not set\n");
                free(prev_cwd);
                return 1;
            }
            targetdir = cmd->oldpwd;
        } else {
            targetdir = arg;
        }
    }

    if (chdir(targetdir) == -1) {
        perror("smash error: chdir failed");
        free(prev_cwd);
        return 1;
    }
    free(cmd->oldpwd);
    cmd->oldpwd = prev_cwd;

    return 0;
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
        if (max == NULL || cmd_list->jobs_list[i]->id > max->id) {
            max = cmd_list->jobs_list[i];
        }
    }
    return max;
}
void removejobbyid(JobsList* cmd_list, int id) {
    if (!cmd_list) {
        return;
    }
    for (int i = 0; i < JOBS_NUM_MAX; ++i) {
        Command *removeable = cmd_list->jobs_list[i];
        if (removeable && removeable->id == id) {
            cmd_list->jobs_list[i] = NULL;
            free(removeable);
            return;
        }
    }
}
int exec_fg(JobsList* cmd_list, Command* cmd) {
    if (!cmd) {
        fprintf(stderr, "smash error: internal: cmd is NULL in fg\n");
        return 1;
    }
    if (cmd->num_of_args > 1) {
        fprintf(stderr, "smash error: fg: too many arguments\n");
        return 1;
    }

    Command *job = NULL;
    int job_id = -1;

    if (cmd->num_of_args == 1) {
        if (cmd->args[0] == NULL) {
            fprintf(stderr, "smash error: fg: invalid argument\n");
            return 1;
        }
        job_id = atoi(cmd->args[0]);
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
        job_id = job->id;
    }

    // Print the command being brought to foreground
    printf("%s : %d\n", job->full_cmd, job->pid);

    // Send SIGCONT if the job is stopped
    if (job->cmd_status == STOPPED) {
        if (my_system_call(SYS_KILL, job->pid, SIGCONT) == -1) {
            perror("smash error: fg: kill failed");
            return 1;
        }
    }

    // Remove job from list before bringing to foreground
    // (foreground jobs are not tracked in the jobs list)
    removejobbyid(cmd_list, job_id);

    // Wait for the job to complete or stop
    int status;
    pid_t ret = my_system_call(SYS_WAITPID, job->pid, &status, WUNTRACED);
    if (ret == -1) {
        perror("smash error: fg: waitpid failed");
        free(job);
        return 1;
    }

    // If job was stopped (Ctrl+Z), add it back to jobs list
    if (WIFSTOPPED(status)) {
        job->cmd_status = STOPPED;
        job->start_time = time(NULL);
        AddCommand(cmd_list, job);
    } else {
        // Job completed or was terminated - free the memory
        free(job->oldpwd);
        free(job);
    }

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

    for (int i = 0; i < JOBS_NUM_MAX; ++i) {
        if (cmd_list->jobs_list[i]) {
            Command* job = cmd_list->jobs_list[i];
            time_t elapsed = time(NULL) - job->start_time;
            printf("[%d] %s : %d %ld secs",
                   job->id,
                   job->full_cmd,
                   job->pid,
                   (long)elapsed);
            if (job->cmd_status == STOPPED) {
                printf(" (stopped)");
            }
            printf("\n");
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

    // args[0] should be -<signal>, args[1] should be <job-id>
    if (!cmd->args[0] || !cmd->args[1]) {
        fprintf(stderr, "smash error: kill: invalid arguments\n");
        return;
    }

    if (cmd->args[0][0] != '-') {
        fprintf(stderr, "smash error: kill: invalid arguments\n");
        return;
    }

    int sig = atoi(cmd->args[0] + 1); // skip the '-'
    int job_id = atoi(cmd->args[1]);

    Command* job = findjobbyid(cmd_list, job_id);
    if (!job) {
        fprintf(stderr, "smash error: kill: job-id %d does not exist\n", job_id);
        return;
    }

    if (my_system_call(SYS_KILL, job->pid, sig) == -1) {
        perror("smash error: kill failed");
        return;
    }

    printf("signal number %d was sent to pid %d\n", sig, job->pid);
}

void exec_bg(JobsList* cmd_list, Command* cmd) {
    if (!cmd) {
        fprintf(stderr, "smash error: internal: cmd is NULL in bg\n");
        return;
    }

    if (cmd->num_of_args > 1) {
        fprintf(stderr, "smash error: bg: invalid arguments\n");
        return;
    }

    Command *job = NULL;
    if (cmd->num_of_args == 1) {
        if (cmd->args[0] == NULL) {
            fprintf(stderr, "smash error: bg: invalid arguments\n");
            return;
        }
        int job_id = atoi(cmd->args[0]);
        job = findjobbyid(cmd_list, job_id);
        if (!job) {
            fprintf(stderr, "smash error: bg: job-id %d does not exist\n", job_id);
            return;
        }
    } else {
        job = findmaxjobid(cmd_list);
        if (!job) {
            fprintf(stderr, "smash error: bg: there is no stopped jobs to resume\n");
            return;
        }
    }

    if (job->cmd_status != STOPPED) {
        fprintf(stderr, "smash error: bg: job-id %d is already running in the background\n", job->id);
        return;
    }

    if (my_system_call(SYS_KILL, job->pid, SIGCONT) == -1) {
        perror("smash error: bg: kill failed");
        return;
    }

    job->cmd_status = BACKGROUND;
    printf("%s : %d\n", job->full_cmd, job->pid);
}

void exec_quit(JobsList* cmd_list, Command* cmd) {
    if (!cmd) {
        fprintf(stderr, "smash error: internal: cmd is NULL in quit\n");
        return;
    }

    if (cmd->num_of_args > 1) {
        fprintf(stderr, "smash error: quit: invalid arguments\n");
        return;
    }

    int kill_all = 0;
    if (cmd->num_of_args == 1 && strcmp(cmd->args[0], "kill") == 0) {
        kill_all = 1;
    }

    if (kill_all) {
        printf("smash: sending SIGKILL signal to %d jobs:\n", JOBS_NUM_MAX);
        for (int i = 0; i < JOBS_NUM_MAX; ++i) {
            if (cmd_list->jobs_list[i]) {
                Command* job = cmd_list->jobs_list[i];
                printf("%d: %s\n", job->pid, job->full_cmd);
                my_system_call(SYS_KILL, job->pid, SIGKILL);
            }
        }
    }

    exit(0);
}

void exec_diff(JobsList* cmd_list, Command* cmd) {
    if (!cmd) {
        fprintf(stderr, "smash error: internal: cmd is NULL in diff\n");
        return;
    }

    if (cmd->num_of_args != 2) {
        fprintf(stderr, "smash error: diff: invalid arguments\n");
        return;
    }

    // This is a placeholder - actual implementation would use fork/exec
    // to run the diff command
    fprintf(stderr, "smash error: diff: not implemented\n");
}


