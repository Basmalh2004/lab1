//smash.c

/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <stdlib.h>

#include "commands.h"
#include "signals.h"

/*=============================================================================
* classes/structs declarations
=============================================================================*/
 
/*=============================================================================
* global variables & data structures
=============================================================================*/
char _line[CMD_LENGTH_MAX];
Job jobs_list[JOBS_NUM_MAX];
char old_pwd[CMD_LENGTH_MAX] = ""; // Store previous working directory


/*=============================================================================
* main function
=============================================================================*/
int main(int argc, char* argv[])
{
	// Initialize jobs list
	for(int i = 0; i < JOBS_NUM_MAX; i++){
		jobs_list[i].jobId = 0;  // 0 means empty slot
		jobs_list[i].jobPid = -1;
		jobs_list[i].status = FOREGROUND;
	}

	// Setup signal handlers
	setupSignalHandlers();

	char _cmd[CMD_LENGTH_MAX];
	while(1) {
		printf("basmalamhmdshell > ");
		fgets(_line, CMD_LENGTH_MAX, stdin);
		strcpy(_cmd, _line);

		//execute command
		Command cmd;
		if (parseCmdExample(_cmd, &cmd) == VALID_COMMAND){
			command_handler(&cmd);
		}

		//initialize buffers for next command
		_line[0] = '\0';
		_cmd[0] = '\0';
	}

	return 0;
}
