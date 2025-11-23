// signals.c
#include "signals.h"
#include "commands.h"
#include <string.h>

// Global variables to track foreground process
pid_t fgProcessPid = -1;
char fgProcessCmd[CMD_LENGTH_MAX] = "";
time_t fgProcessStartTime = 0;

// SIGINT handler (CTRL+C)
void handleSIGINT(int sig) {
    printf("\n");
    if (fgProcessPid > 0) {
        // There's a foreground process - kill it
        if (my_system_call(SYS_KILL, fgProcessPid, SIGINT) == 0) {
            printf("basmalamhmdshell: process %d was killed\n", fgProcessPid);
        }
        fgProcessPid = -1;
    } else {
        // No foreground process
        printf("basmalamhmdshell: caught CTRL+C\n");
        printf("basmalamhmdshell > ");
        fflush(stdout);
    }
}

// SIGTSTP handler (CTRL+Z)
void handleSIGTSTP(int sig) {
    printf("\n");
    if (fgProcessPid > 0) {
        // There's a foreground process - stop it
        if (my_system_call(SYS_KILL, fgProcessPid, SIGSTOP) == 0) {
            printf("basmalamhmdshell: process %d was stopped\n", fgProcessPid);

            // Add stopped job to jobs list
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

            // Add to first empty slot
            for (int i = 0; i < JOBS_NUM_MAX; i++) {
                if (jobs_list[i].jobId == 0) {
                    jobs_list[i].jobId = newJobId;
                    jobs_list[i].jobPid = fgProcessPid;
                    strncpy(jobs_list[i].jobCmd_str, fgProcessCmd, CMD_LENGTH_MAX - 1);
                    jobs_list[i].status = STOPPED;
                    jobs_list[i].startTime = fgProcessStartTime;
                    break;
                }
            }
        }
        fgProcessPid = -1;
    } else {
        // No foreground process
        printf("basmalamhmdshell: caught CTRL+Z\n");
        printf("basmalamhmdshell > ");
        fflush(stdout);
    }
}

// Setup signal handlers
void setupSignalHandlers() {
    my_system_call(SYS_SIGNAL, SIGINT, (void*)handleSIGINT);
    my_system_call(SYS_SIGNAL, SIGTSTP, (void*)handleSIGTSTP);
}