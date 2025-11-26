//smash.c

/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "commands.h"
#include "signals.h"

/*=============================================================================
* classes/structs declarations
=============================================================================*/

/*=============================================================================
* global variables & data structures
=============================================================================*/
char _line[CMD_LENGTH_MAX];
JobsList* jobs = NULL;

/*=============================================================================
* main function
=============================================================================*/
int main(int argc, char* argv[])
{
    char _cmd[CMD_LENGTH_MAX];

    // Initialize jobs list
    jobs = Init_Jobs();
    if (!jobs) {
        fprintf(stderr, "smash error: failed to initialize jobs list\n");
        return 1;
    }

    while(1) {
        printf("smash > ");
        if (!fgets(_line, CMD_LENGTH_MAX, stdin)) {
            break; // EOF or error
        }

        strcpy(_cmd, _line);
        _cmd[CMD_LENGTH_MAX-1] = '\0';

        //execute command
        Command* cmd_res = ParseCmd(_cmd);
        if (!cmd_res) {
            // Invalid command or empty line
            _line[0] = '\0';
            _cmd[0] = '\0';
            continue;
        }

        // Check if it's a background command and add to jobs list
        if (cmd_res->cmd_status == BACKGROUND) {
            AddCommand(jobs, cmd_res);
        }

        // Execute the command
        CheckStatus(jobs, cmd_res);

        //initialize buffers for next command
        _line[0] = '\0';
        _cmd[0] = '\0';
    }

    return 0;
}
