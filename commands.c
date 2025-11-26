//commands.c
#include "commands.h"

#include "my_system_call.h"
#include "unistd.h"





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
    cmd_ptr->start_time = 0; //RB----------------------------
    cmd_ptr->id = INIT_ID;
    cmd_ptr->pid = getpid();

    char* delimiters = " \t\n"; //parsing should be done by spaces, tabs or newlines
    char* cmd = strtok(line, delimiters); //read strtok documentation - parses string by delimiters
    strcpy(cmd_ptr->cmd, cmd);

    if(!cmd)
        return INVALID_COMMAND; //this means no tokens were found, most like since command is invalid

    char* args[MAX_ARGS];
    int nargs = 0;

    args[0] = cmd; //first token before spaces/tabs/newlines should be command name

    for(int i = 1; i < MAX_ARGS; i++)
    {
        args[i] = strtok(NULL, delimiters); //first arg NULL -> keep tokenizing from previous call
        if(!args[i])
            break;
        nargs++;

        if (strcmp(args[i], "&") == 0) {
            args[i] = NULL;
            nargs--;
            cmd_ptr->cmd_status = BACKGROUND;
        }
    }
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
    if (cmd->num_of_args == 1) {
        if (cmd->args[0] == NULL) {
            fprintf(stderr, "smash error: fg: invalid argument\n");
            return 1;
        }
        int job_id = atoi(cmd->args[0]);
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

    if (job->cmd_status == STOPPED) {
        if (my_system_call(SYS_KILL, job->pid, SIGCONT) == -1) {
            perror("smash error: fg: kill failed");
            return 1;
        }
    }

    job->start_time = time(NULL);
    job->cmd_status = FOREGROUND;

    int status;
    pid_t ret = my_system_call(SYS_WAITPID, job->pid, &status, WUNTRACED);
    if (ret == -1) {
        perror("smash error: fg: waitpid failed");
        return 1;
    }

    if (WIFSTOPPED(status)) {
        job->cmd_status = STOPPED;
        job->start_time = time(NULL);
    } else {
        removejobbyid(cmd_list, job->id);
    }

    return 0;
}


