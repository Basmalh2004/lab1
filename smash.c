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

// Alias storage (Alias type defined in commands.h)
Alias aliases[MAX_ALIASES];
int num_aliases = 0;

/*=============================================================================
* helper functions
=============================================================================*/

// Find alias by name, returns -1 if not found
int find_alias(const char* name) {
    for (int i = 0; i < num_aliases; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Expand alias if command matches an alias name
void expand_alias(char* cmd_line) {
    // Get first word (command name)
    char temp[CMD_LENGTH_MAX];
    strcpy(temp, cmd_line);
    char* first_word = strtok(temp, " \t\n");
    if (!first_word) return;

    int idx = find_alias(first_word);
    if (idx >= 0) {
        // Found alias - expand it
        char expanded[CMD_LENGTH_MAX * 2];
        char* rest = cmd_line + strlen(first_word);
        // Skip whitespace after command name
        while (*rest == ' ' || *rest == '\t') rest++;

        snprintf(expanded, sizeof(expanded), "%s %s", aliases[idx].command, rest);
        strncpy(cmd_line, expanded, CMD_LENGTH_MAX - 1);
        cmd_line[CMD_LENGTH_MAX - 1] = '\0';
    }
}

// Execute a single command, return result
int execute_single_command(char* cmd_str) {
    char cmd_copy[CMD_LENGTH_MAX];
    strcpy(cmd_copy, cmd_str);

    // Expand alias
    expand_alias(cmd_copy);

    Command cmd;
    if (parseCmdExample(cmd_copy, &cmd) == VALID_COMMAND) {
        cleanup_finished_jobs();
        return command_handler(&cmd);
    }
    return SMASH_SUCCESS;  // Empty command is success
}

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
		if (fgets(_line, CMD_LENGTH_MAX, stdin) == NULL) {
			// EOF received
			break;
		}
		strcpy(_cmd, _line);

		// Remove trailing newline
		size_t len = strlen(_cmd);
		if (len > 0 && _cmd[len-1] == '\n') {
			_cmd[len-1] = '\0';
		}

		// Handle compound commands with &&
		char* cmd_ptr = _cmd;
		char* and_pos;
		int last_result = SMASH_SUCCESS;

		while ((and_pos = strstr(cmd_ptr, "&&")) != NULL) {
			// Extract command before &&
			char single_cmd[CMD_LENGTH_MAX];
			int cmd_len = and_pos - cmd_ptr;
			strncpy(single_cmd, cmd_ptr, cmd_len);
			single_cmd[cmd_len] = '\0';

			// Trim trailing whitespace
			while (cmd_len > 0 && (single_cmd[cmd_len-1] == ' ' || single_cmd[cmd_len-1] == '\t')) {
				single_cmd[--cmd_len] = '\0';
			}

			// Execute the command
			last_result = execute_single_command(single_cmd);

			// If command failed, don't execute the rest
			if (last_result != SMASH_SUCCESS) {
				goto done;
			}

			// Move to next command (skip &&)
			cmd_ptr = and_pos + 2;
			// Skip leading whitespace
			while (*cmd_ptr == ' ' || *cmd_ptr == '\t') cmd_ptr++;
		}

		// Execute remaining/only command
		if (*cmd_ptr != '\0') {
			execute_single_command(cmd_ptr);
		}

done:
		//initialize buffers for next command
		_line[0] = '\0';
		_cmd[0] = '\0';
	}

	return 0;
}
