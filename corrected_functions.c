// Corrected helper functions for job management

// Find a job by its ID
Job* findjobbyid(int id) {
    for (int i = 0; i < JOBS_NUM_MAX; ++i) {
        if (jobs_list[i].jobId != 0 && jobs_list[i].jobId == id) {
            return &jobs_list[i];
        }
    }
    return NULL;  // Job not found
}

// Find the job with the maximum job ID
Job* findmaxjobid() {
    Job* max = NULL;
    for (int i = 0; i < JOBS_NUM_MAX; ++i) {
        if (jobs_list[i].jobId == 0) {
            continue;
        }
        if (max == NULL || jobs_list[i].jobId > max->jobId) {
            max = &jobs_list[i];
        }
    }
    return max;
}

// Remove a job by its ID
void removejobbyid(int id) {
    for (int i = 0; i < JOBS_NUM_MAX; ++i) {
        Job* removeable = &jobs_list[i];
        if (removeable->jobId == id) {
            // Clear the job entry
            removeable->jobId = 0;
            removeable->jobPid = 0;
            removeable->jobCmd_str[0] = '\0';
            removeable->status = FOREGROUND;
            removeable->startTime = 0;
            return;
        }
    }
}

// Execute fg command (simplified version)
int exec_fg(Command* cmd) {
    if (!cmd) {
        perrorSmash("fg", "invalid command");
        return SMASH_FAIL;
    }

    if (cmd->nargs > 2) {
        perrorSmash("fg", "invalid arguments");
        return SMASH_FAIL;
    }

    Job* job = NULL;

    if (cmd->nargs == 2) {
        // fg with job ID specified
        int job_id = atoi(cmd->args[1]);
        job = findjobbyid(job_id);
        if (!job) {
            char msg[80];
            snprintf(msg, 80, "job id %d does not exist", job_id);
            perrorSmash("fg", msg);
            return SMASH_FAIL;
        }
    } else if (cmd->nargs == 1) {
        // fg without arguments - use highest job ID
        job = findmaxjobid();
        if (!job) {
            perrorSmash("fg", "jobs list is empty");
            return SMASH_FAIL;
        }
    }

    // Print job info
    printf("[%d] %s\n", job->jobId, job->jobCmd_str);

    // Set terminal control to the job's process group
    tcsetpgrp(STDIN_FILENO, job->jobPid);

    // If job is stopped, send SIGCONT to resume it
    if (job->status == STOPPED) {
        if (my_system_call(SYS_KILL, job->jobPid, SIGCONT) == -1) {
            perrorSmash("fg", "failed to send SIGCONT");
            tcsetpgrp(STDIN_FILENO, getpid());
            return SMASH_FAIL;
        }
    }

    // Update job status and time
    job->status = FOREGROUND;
    job->startTime = time(NULL);

    // Wait for the job to complete or stop
    int status;
    pid_t ret = my_system_call(SYS_WAITPID, job->jobPid, &status, WUNTRACED);

    // Restore terminal control to shell
    tcsetpgrp(STDIN_FILENO, getpid());

    if (ret == -1) {
        perrorSmash("fg", "waitpid failed");
        return SMASH_FAIL;
    }

    if (WIFSTOPPED(status)) {
        // Job was stopped again (e.g., by Ctrl+Z)
        job->status = STOPPED;
        job->startTime = time(NULL);
        return SMASH_SUCCESS;
    } else {
        // Job completed - remove from jobs list
        removejobbyid(job->jobId);
        return SMASH_SUCCESS;
    }
}
