#ifndef SIGNALS_H
#define SIGNALS_H

/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include "my_system_call.h"

#define CMD_LENGTH_MAX 120  // From commands.h

/*=============================================================================
* global variables
=============================================================================*/
extern pid_t fgProcessPid;
extern char fgProcessCmd[CMD_LENGTH_MAX];
extern time_t fgProcessStartTime;

/*=============================================================================
* global functions
=============================================================================*/
void handleSIGINT(int sig);
void handleSIGTSTP(int sig);
void setupSignalHandlers();

#endif //__SIGNALS_H__