#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "error_handling.h"
#include "internal.h"
#include "process_control.h"

#define ARGS_MAX 64

// Split input line into separate arguments
int parse_args(char *input, char **args) {
    int arg_count = 0;
    char *token = strtok(input, " \t");
    while (token != NULL && arg_count < ARGS_MAX) {
        args[arg_count] = token;
        arg_count++;
        token = strtok(NULL, " \t");
    }
    args[arg_count] = NULL;
    return arg_count;
}

// handling input function
// returns: 0 = empty, 1 = exit command, 2 = internal command, 3 = external command
int handle_input(char *input, char **args, int *arg_count) {
    
    // Strip the newline character from input
    input[strcspn(input, "\n")] = '\0';

    // Handle empty lines or whitespace-only input
    if (input[0] == '\0' || strspn(input, " \t") == strlen(input)) {
        return 0; // empty, continue
    }

    // Break command into tokens
    *arg_count = parse_args(input, args);

    if (*arg_count == 0) {
        return 0; // empty after parsing
    }

    // Identify what type of command this is
    if (strcmp(args[0], "exit") == 0) {
        return 1; // exit command
    }
    
    if (strcmp(args[0], "cd") == 0 || strcmp(args[0], "jobs") == 0) {
        return 2; // internal command
    }

    return 3; // external command
}

int main(void) {
    char command[MAX_LINE];
    // We need a clean copy of the command line for the jobs list because strtok destroys 'command'
    char command_copy[MAX_LINE]; 
    
    while(1) {
        // Display prompt and wait for input
        printf("hw1shell$ ");
        fflush(stdout);

        // Read user command
        if (fgets(command, sizeof(command), stdin) == NULL) { 
            printf("\n");
            cleanup_jobs();
            break; 
        }

        // Save original command for job tracking (strtok will modify command)
        strcpy(command_copy, command);
        // Remove newline from copy manually for the jobs display
        command_copy[strcspn(command_copy, "\n")] = '\0';

        char *args[ARGS_MAX]; 
        int arg_count;
        int result = handle_input(command, args, &arg_count);
        
        if (result == 1) {
            // User entered 'exit'
            cleanup_jobs();
            break; 
        } 
        
        if (result == 2) {
            // Built-in command (cd or jobs)
            execute_internal(args, arg_count);
        } else if (result == 3) { 
            // External command
            int is_background = 0;

            // Check if the last argument is "&"
            if (arg_count > 0 && strcmp(args[arg_count - 1], "&") == 0) {
                is_background = 1;
                args[arg_count - 1] = NULL;
            }

            execute_external_command(args, is_background, command_copy);
        }
        // result == 0 (empty input) falls through to reaping

        // Check for finished background jobs after every iteration
        check_and_reap_background_jobs();
    }
    
    return 0;
}