//smash.c

/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "signals.h"

/*=============================================================================
* classes/structs declarations
=============================================================================*/

/*=============================================================================
* global variables & data structures
=============================================================================*/
char _line[CMD_LENGTH_MAX];
char *oldpwd;
/*=============================================================================
* main function
=============================================================================*/
int main(int argc, char* argv[])
{
    char _cmd[CMD_LENGTH_MAX];
    JobsList* jobs = Init_Jobs();

    while(1) {
        // init to jobs
        printf("smash > ");
        fgets(_line, CMD_LENGTH_MAX, stdin);
        strcpy(_cmd, _line);
        _cmd[CMD_LENGTH_MAX-1] = '\0';
        //execute command
        Command* cmd_res = ParseCmd(_cmd);
        if (cmd_res && cmd_res->cmd_status == BACKGROUND) {
            AddCommand(jobs,cmd_res);
        }

        //initialize buffers for next command
        _line[0] = '\0';
        _cmd[0] = '\0';
    }

    return 0;
}
