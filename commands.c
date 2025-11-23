//commands.c
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include "commands.h"

void wait_or_kill(pid_t pid) {
    int status;
    time_t start_time = time(NULL);
    time_t elapsed = 0;
    int finished = 0;

    while (elapsed < 5) {
        // Non-blocking wait
        int ret = my_system_call(SYS_WAITPID, pid, &status, WNOHANG);
        if (ret > 0) {
            finished = 1;
            break;
        }
        // Sleep for 100ms using nanosleep (more compatible than usleep)
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 100000000; // 100 ms in nanoseconds
        nanosleep(&ts, NULL);
        elapsed = difftime(time(NULL), start_time);
    }

    if (finished) {
        if (WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM) {
            printf("done\n"); // killed by SIGTERM
        } else if (WIFEXITED(status)) {
            printf("done\n"); // exited normally
        }
    } else {
        printf("sending SIGKILL... ");
        my_system_call(SYS_KILL, pid, SIGKILL);
        my_system_call(SYS_WAITPID, pid, &status, 0);
        printf("done\n");
    }
}

// Function to build command string from Command struct
void buildCommandString(Command* cmd, char* result, int max_size) {
    int pos = 0;

    // add the command itself
    pos += snprintf(result + pos, max_size - pos, "%s", cmd->cmd);

    // add each argument with a single space (skip args[0] which is cmd name)
    for (int i = 1; i < cmd->nargs; i++) {
        if (cmd->args[i] != NULL) {
            pos += snprintf(result + pos, max_size - pos, " %s", cmd->args[i]);
        }
    }

    result[pos] = '\0';
}

//example function for printing errors from internal commands
void perrorSmash(const char* cmd, const char* msg)
{
    fprintf(stderr, "basmalamhmdshell error: %s%s%s\n",
        cmd ? cmd : "",
        cmd ? ": " : "",
        msg);
}

//example function for parsing commands
int parseCmdExample(char* line , Command* cmd_t)
{
	const char* delimiters = " \t\n"; // to divide
	char* cmd = strtok(line, delimiters); //strtok
	if(!cmd)
		return INVALID_COMMAND;
	
	char* args[MAX_ARGS];
	int nargs = 0;
	args[0] = cmd; //first token before spaces/tabs/newlines
	for(int i = 1; i < MAX_ARGS; i++)
	{
		args[i] = strtok(NULL, delimiters); //first arg NULL
		if(!args[i])
			break;
		nargs++;
	}
	cmd_t->cmd = cmd;
	cmd_t->isBackground = 0;  // Default: foreground

	// Check if last argument is '&' (background)
	if (nargs > 0 && args[nargs] && strcmp(args[nargs], "&") == 0) {
		cmd_t->isBackground = 1;
		args[nargs] = NULL;  // Remove '&' from args
		nargs--;  // Decrease arg count
	}

	for (int i = 0; i <= nargs; i++) {
		cmd_t->args[i] = args[i];
	}
    //nargs all astagfrallah
	cmd_t->nargs = nargs + 1; // including command itself
	return VALID_COMMAND;
}


// Clean up finished background jobs from the list
// Per spec page 4: must be called before running any command,
// before printing jobs list, and before adding new jobs
void cleanup_finished_jobs(){
	for(int i=0; i<JOBS_NUM_MAX; i++){
		if (jobs_list[i].jobId != 0 && jobs_list[i].status == BACKGROUND){
			int status;
			// Non-blocking check if process has terminated
			int result = my_system_call(SYS_WAITPID, jobs_list[i].jobPid, &status, WNOHANG);
			if (result > 0) {
				// Process has terminated, remove from list
				jobs_list[i].jobId = 0;
				jobs_list[i].jobPid = 0;
				jobs_list[i].jobCmd_str[0] = '\0';
				jobs_list[i].status = FOREGROUND;
			}
		}
	}
}

void jobes_list_print(){
	cleanup_finished_jobs();  // Clean up before printing
	time_t now ;
	time_t diff;
	for(int i=0; i<JOBS_NUM_MAX; i++){
		if (jobs_list[i].jobId !=0){
			now = time (NULL);
			diff = now - jobs_list[i].startTime;
			if(jobs_list[i].status == BACKGROUND)
			    printf("[%d] %s: %d %d secs\n", jobs_list[i].jobId, jobs_list[i].jobCmd_str, jobs_list[i].jobPid, (int)diff);
			else if (jobs_list[i].status == STOPPED)
			    printf("[%d] %s: %d %d secs (stopped)\n", jobs_list[i].jobId, jobs_list[i].jobCmd_str, jobs_list[i].jobPid, (int)diff);
		}
	}
}

int kill_signal(int signum , int jobId){
	int jid = jobId;
    char msg[80];
    snprintf(msg, 80, "job id %d does not exist", jid);
	pid_t pid = -1;
	for(int i=0; i<JOBS_NUM_MAX; i++){
		if (jobs_list[i].jobId == jobId){
			pid = jobs_list[i].jobPid;
			break;
		}
	}
	if (pid == -1){
		perrorSmash("kill", msg);
		return -1;
	}
	if (my_system_call(SYS_KILL, pid, signum) >= 0){
		printf("signal %d was sent to pid %d\n", signum, pid);
		return SMASH_SUCCESS;
	}
	return SMASH_FAIL;
}


int fg_command(int jobId){
	int jid = jobId;
	pid_t shell_pid = getpid();
	char msg[80];
	snprintf(msg, 80, "job id %d does not exist", jid);
	pid_t pid = -1;
	Job* job = NULL;
	for(int i=0; i<JOBS_NUM_MAX; i++){
		if (jobs_list[i].jobId == jobId){
			pid = jobs_list[i].jobPid;
			job = &jobs_list[i];
			break;
		}
	}
	if (pid == -1){
		perrorSmash("fg", msg);
		return -1;
	}

	// Print job info
	printf("[%d] %s\n", job->jobId, job->jobCmd_str);

	tcsetpgrp(STDIN_FILENO, pid);


    if (job->status == STOPPED)
        my_system_call(SYS_KILL, pid, SIGCONT);

    int status;
    my_system_call(SYS_WAITPID, pid, &status, WUNTRACED);
	tcsetpgrp(STDIN_FILENO, shell_pid);
	if (WIFSTOPPED(status))
		return STILL_STOPPED;

	return SMASH_SUCCESS;
}

int bg_command(int jobId){
	int jid = jobId;
	char msg1[80], msg2[80];
	snprintf(msg1, 80, "job id %d does not exist", jid);
	snprintf(msg2, 80, "job id %d is already running in the background", jid);
	pid_t pid = -1;
	Job* job = NULL;
	for(int i=0; i<JOBS_NUM_MAX; i++){
		if (jobs_list[i].jobId == jobId){
			pid = jobs_list[i].jobPid;
			job = &jobs_list[i];
			break;
		}
	}

	if (pid == -1 ){
		perrorSmash("bg", msg1);
		return SMASH_FAIL;
	}

	// Check if job is already running in background
	if (job->status == BACKGROUND){
		perrorSmash("bg", msg2);
		return SMASH_FAIL;
	}

	// Check if job is stopped (must be stopped to resume)
	if (job->status != STOPPED){
		perrorSmash("bg", "job is not stopped");
		return SMASH_FAIL;
	}

	// Resume the job in background
	my_system_call(SYS_KILL, pid, SIGCONT);
	job->status = BACKGROUND;
	printf("%s: %d\n", job->jobCmd_str, job->jobPid);
	return SMASH_SUCCESS;
}

int quit_command(int kill_jobs){
	// Check if there are any jobs in the list
	int hasJobs = 0;
	for(int i=0; i<JOBS_NUM_MAX; i++){
		if (jobs_list[i].jobId > 0){
			hasJobs = 1;
			break;
		}
	}

	// If not killing jobs and there are jobs, refuse to exit
	if (!kill_jobs && hasJobs){
		perrorSmash("quit", "there are stopped jobs");
		return SMASH_FAIL;
	}

	// If killing jobs, send SIGTERM to all jobs
	if (kill_jobs){
		for(int i=0; i<JOBS_NUM_MAX; i++){
			if (jobs_list[i].jobId > 0){
				printf("[%d] %s - sending SIGTERM... ", jobs_list[i].jobId, jobs_list[i].jobCmd_str);
				my_system_call(SYS_KILL, jobs_list[i].jobPid, SIGTERM);
				wait_or_kill(jobs_list[i].jobPid);  // wait_or_kill already prints "done"
			}
		}
	}

	exit(0);
	return SMASH_SUCCESS;
}


int diff_cmd(char* path1, char* path2) {
    struct stat st1, st2;
	// 1. Check paths are different
    if (stat(path1, &st1) != 0 || stat(path2, &st2) != 0) {
        perrorSmash("diff", "expected valid paths for files");
        return SMASH_FAIL;
    }

    // 3. Check not directories
    if (S_ISDIR(st1.st_mode) || S_ISDIR(st2.st_mode)) {
        perrorSmash("diff", "paths are not files");
        return SMASH_FAIL;
    }

    // 4. Open both files
    int fd1 = my_system_call(SYS_OPEN, path1, O_RDONLY);
    int fd2 = my_system_call(SYS_OPEN, path2, O_RDONLY);

    if (fd1 < 0 || fd2 < 0) {
        perrorSmash("diff", "expected valid paths for files");
        return SMASH_FAIL;
    }

    char buf1[1024], buf2[1024];
    int r1, r2;

    while (1) {
        r1 = my_system_call(SYS_READ, fd1, buf1, sizeof(buf1));
        r2 = my_system_call(SYS_READ, fd2, buf2, sizeof(buf2));

        if (r1 != r2) {
            printf("1\n");
            break;
        }

        if (r1 == 0) {  // end of file both
            printf("0\n");
            break;
        }

        if (memcmp(buf1, buf2, r1) != 0) {
            printf("1\n");
            break;
        }
    }

    my_system_call(SYS_CLOSE, fd1);
    my_system_call(SYS_CLOSE, fd2);

    return SMASH_SUCCESS;
}

// External command execution
int executeExternal(Command* cmd) {
    // Validate command
    if (!cmd->cmd || cmd->cmd[0] == '\0') {
        perrorSmash("external", "invalid command");
        return SMASH_FAIL;
    }

    // Fork child process
    pid_t pid = my_system_call(SYS_FORK);
    if (pid == -1) {
        perrorSmash("fork", "failed");
        return SMASH_FAIL;
    }

    // ===== CHILD PROCESS =====
    if (pid == 0) {
        setpgrp();  // New process group for signal isolation

        // Build args array: [cmd->args[0], cmd->args[1], ..., NULL]
        // Note: cmd->args already contains cmd name at index 0
        char* args[ARGS_NUM_MAX + 1];
        for (int i = 0; i < cmd->nargs; i++) {
            args[i] = cmd->args[i];
        }
        args[cmd->nargs] = NULL;

        // Execute the external command
        my_system_call(SYS_EXECVP, cmd->cmd, args);

        // If we get here, exec failed
        perrorSmash(cmd->cmd, "command not found");
        exit(1);
    }

    // ===== PARENT PROCESS =====

    // Check if command should run in background
    if (cmd->isBackground) {
        // Background: add to jobs list and return immediately
        cleanup_finished_jobs();  // Clean up before adding new job (per spec page 4)

        // Build command string for job list
        char cmdStr[CMD_LENGTH_MAX];
        buildCommandString(cmd, cmdStr, CMD_LENGTH_MAX);
        strcat(cmdStr, " &");  // Add & to show it's background

        // Find minimum available job ID (per spec page 4)
        int newJobId = 1;
        int found = 1;
        while (found) {
            found = 0;
            for (int i = 0; i < JOBS_NUM_MAX; i++) {
                if (jobs_list[i].jobId == newJobId) {
                    found = 1;
                    newJobId++;
                    break;
                }
            }
        }

        // Add to jobs list with BACKGROUND status
        for (int i = 0; i < JOBS_NUM_MAX; i++) {
            if (jobs_list[i].jobId == 0) {
                jobs_list[i].jobId = newJobId;
                jobs_list[i].jobPid = pid;
                strncpy(jobs_list[i].jobCmd_str, cmdStr, CMD_LENGTH_MAX - 1);
                jobs_list[i].status = BACKGROUND;
                jobs_list[i].startTime = time(NULL);
                break;
            }
        }

        return SMASH_SUCCESS;  // Don't wait!
    }

    // Foreground: wait for process to complete
    fgProcessPid = pid;
    fgProcessStartTime = time(NULL);

    // Build command string for tracking
    buildCommandString(cmd, fgProcessCmd, CMD_LENGTH_MAX);

    int status;
    my_system_call(SYS_WAITPID, pid, &status, WUNTRACED);
    fgProcessPid = -1;

    // If process was stopped (by CTRL+Z), return success
    // (The signal handler already added it to jobs_list)
    if (WIFSTOPPED(status)) {
        return SMASH_SUCCESS;
    }

    // Per spec page 15: external command fails if return value is not 0
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        return SMASH_FAIL;
    }

    return SMASH_SUCCESS;
}

//cmd->narg function that total count of strings not a letters
int command_handler(Command* cmd){
	// showpid command
	if (strcmp(cmd->cmd, "showpid") == 0){
		if (cmd->nargs != 1){
			perrorSmash("showpid", "expected 0 arguments");
			return SMASH_FAIL;
		}
		printf("smash pid is %d\n", getpid());
		return SMASH_SUCCESS;
	}

	// pwd command
    //the important thing to use getcwd and perror to error massages
	else if (strcmp(cmd->cmd, "pwd") == 0){
		if (cmd->nargs != 1){
			perrorSmash("pwd", "expected 0 arguments");
			return SMASH_FAIL;
		}
		char cwd[CMD_LENGTH_MAX];
		if (getcwd(cwd, sizeof(cwd)) != NULL){
			printf("%s\n", cwd);
			return SMASH_SUCCESS;
		}
		else{
			perrorSmash("pwd", "failed to get current directory");
			return SMASH_FAIL;
		}
	}

	// cd command
	else if (strcmp(cmd->cmd, "cd") == 0){
		if (cmd->nargs != 2){
			perrorSmash("cd", "expected 1 arguments");
			return SMASH_FAIL;
		}

		char* target_path = cmd->args[1];
		char temp_path[CMD_LENGTH_MAX];
// we want to save the previous one for this command so let me save the second one of arg(done)
		// Handle "cd -" to go to previous directory
		if (strcmp(target_path, "-") == 0){
			if (old_pwd[0] == '\0'){
                //let me make sure we can put this(check it )
				perrorSmash("cd", "old pwd not set");
				return SMASH_FAIL;
			}
			strcpy(temp_path, old_pwd);
			target_path = temp_path;
			printf("%s\n", target_path);  // Print the path after cd -
		}

		// Check if target exists and is a directory
		struct stat st;
		if (stat(target_path, &st) != 0){
			perrorSmash("cd", "target directory does not exist");
			return SMASH_FAIL;
		}

		if (!S_ISDIR(st.st_mode)){
			perrorSmash("cd", "not a directory");
			return SMASH_FAIL;
		}

		// Save current directory to old_pwd before changing we can actually do
		char current_dir[CMD_LENGTH_MAX];
		if (getcwd(current_dir, sizeof(current_dir)) != NULL){
			strcpy(old_pwd, current_dir);
		}

		// Change directory
		if (chdir(target_path) != 0){
			perrorSmash("cd", "failed to change directory");
			return SMASH_FAIL;
		}

		return SMASH_SUCCESS;
	}

	// jobs command
	else if (strcmp(cmd->cmd, "jobs") == 0){
		if (cmd->nargs != 1){
			perrorSmash("jobs", "expected 0 arguments");
			return SMASH_FAIL;
		}
		jobes_list_print();
		return SMASH_SUCCESS;
	}

	// kill command
	else if (strcmp(cmd->cmd, "kill") == 0){
		if (cmd->nargs != 3){
			perrorSmash("kill", "invalid arguments");
			return SMASH_FAIL;
		}
		int signum = atoi(cmd->args[1]);
		int jobId = atoi(cmd->args[2]);
		return kill_signal(signum, jobId);
	}

	// fg command
	else if (strcmp(cmd->cmd, "fg") == 0){
		if (cmd->nargs > 2){
			perrorSmash("fg", "invalid arguments");
			return SMASH_FAIL;
		}

		int jobId = -1;

		if (cmd->nargs == 2) {
			// fg <job_id> - use specified job
			jobId = atoi(cmd->args[1]);
		} else if (cmd->nargs == 1) {
			// fg (no args) - use most recent job (highest job ID)
			int maxId = -1;
			for (int i = 0; i < JOBS_NUM_MAX; i++) {
				if (jobs_list[i].jobId > 0 && jobs_list[i].jobId > maxId) {
					maxId = jobs_list[i].jobId;
					jobId = maxId;
				}
			}
			if (jobId == -1) {
				perrorSmash("fg", "jobs list is empty");
				return SMASH_FAIL;
			}
		}

		return fg_command(jobId);
	}

	// bg command
	else if (strcmp(cmd->cmd, "bg") == 0){
		if (cmd->nargs > 2){
			perrorSmash("bg", "invalid arguments");
			return SMASH_FAIL;
		}

		int jobId = -1;

		if (cmd->nargs == 2) {
			// bg <job_id> - use specified job
			jobId = atoi(cmd->args[1]);
		} else if (cmd->nargs == 1) {
			// bg (no args) - use most recent stopped job
			int maxId = -1;
			for (int i = 0; i < JOBS_NUM_MAX; i++) {
				if (jobs_list[i].jobId > 0 && jobs_list[i].status == STOPPED && jobs_list[i].jobId > maxId) {
					maxId = jobs_list[i].jobId;
					jobId = maxId;
				}
			}
			if (jobId == -1) {
				perrorSmash("bg", "there are no stopped jobs to resume");
				return SMASH_FAIL;
			}
		}

		return bg_command(jobId);
	}

	// diff command
	else if (strcmp(cmd->cmd, "diff") == 0){
		if (cmd->nargs != 3){
			perrorSmash("diff", "expected 2 arguments");
			return SMASH_FAIL;
		}
		return diff_cmd(cmd->args[1], cmd->args[2]);
	}

	// quit command
	else if( strcmp(cmd->cmd, "quit") == 0){
		if (cmd->nargs == 1){
			return quit_command(0); //no jobs to kill
		}
		else if (cmd->nargs == 2 && strcmp(cmd->args[1], "kill") == 0){
			return quit_command(1); //kill all jobs
		}
		else if (cmd->nargs > 2){
			perrorSmash("quit", "expected 0 or 1 arguments");
			return SMASH_FAIL;
		}
		else{
			perrorSmash("quit", "unexpected arguments");
			return SMASH_FAIL;
		}
	}

	// alias command: alias <name>="<command>"
	else if (strcmp(cmd->cmd, "alias") == 0){
		if (cmd->nargs == 1){
			// Print all aliases
			for (int i = 0; i < num_aliases; i++){
				printf("%s='%s'\n", aliases[i].name, aliases[i].command);
			}
			return SMASH_SUCCESS;
		}

		// Rebuild the full definition string from all args (parser splits on spaces)
		char full_def[CMD_LENGTH_MAX];
		full_def[0] = '\0';
		for (int i = 1; i < cmd->nargs; i++){
			if (i > 1) strcat(full_def, " ");
			strcat(full_def, cmd->args[i]);
		}

		// Parse alias definition: name="command"
		char* eq_pos = strchr(full_def, '=');
		if (!eq_pos){
			perrorSmash("alias", "invalid alias format");
			return SMASH_FAIL;
		}

		// Extract name (before =)
		char name[CMD_LENGTH_MAX];
		int name_len = eq_pos - full_def;
		strncpy(name, full_def, name_len);
		name[name_len] = '\0';

		// Extract command (after =, removing quotes)
		char* cmd_start = eq_pos + 1;
		if (*cmd_start == '"' || *cmd_start == '\''){
			cmd_start++;
		}
		char alias_cmd[CMD_LENGTH_MAX];
		strcpy(alias_cmd, cmd_start);
		int cmd_len = strlen(alias_cmd);
		if (cmd_len > 0 && (alias_cmd[cmd_len-1] == '"' || alias_cmd[cmd_len-1] == '\'')){
			alias_cmd[cmd_len-1] = '\0';
		}

		// Check if alias already exists
		int idx = find_alias(name);
		if (idx >= 0){
			// Update existing alias
			strcpy(aliases[idx].command, alias_cmd);
		} else {
			// Add new alias
			if (num_aliases >= MAX_ALIASES){
				perrorSmash("alias", "too many aliases");
				return SMASH_FAIL;
			}
			strcpy(aliases[num_aliases].name, name);
			strcpy(aliases[num_aliases].command, alias_cmd);
			num_aliases++;
		}
		return SMASH_SUCCESS;
	}

	// unalias command
	else if (strcmp(cmd->cmd, "unalias") == 0){
		if (cmd->nargs != 2){
			perrorSmash("unalias", "expected 1 argument");
			return SMASH_FAIL;
		}

		char* name = cmd->args[1];
		int idx = find_alias(name);
		if (idx < 0){
			perrorSmash("unalias", "alias not found");
			return SMASH_FAIL;
		}

		// Remove alias by shifting array
		for (int i = idx; i < num_aliases - 1; i++){
			strcpy(aliases[i].name, aliases[i+1].name);
			strcpy(aliases[i].command, aliases[i+1].command);
		}
		num_aliases--;
		return SMASH_SUCCESS;
	}

	// Not a built-in command - execute as external command
	else {
		return executeExternal(cmd);
	}
}

// last thing i feel like im talking alone please mohamad i leave "רשתות " in 3 in the morning tpo do this
// i thin
