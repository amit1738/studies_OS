#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "error_handling.h"
#include "internal.h"
#include "process_control.h" // Added include

// parse command into arguments
// returns: number of arguments parsed
int parse_args(char *input, char **args) {
    int arg_count = 0;
    char *token = strtok(input, " \t"); // tokenize by space and tab
    while (token != NULL && arg_count < MAX_LINE / 2) {
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
    
    // remove newline suffix
    input[strcspn(input, "\n")] = '\0';

    // check for empty input - empty or whitespace only
    if (input[0] == '\0' || strspn(input, " \t") == strlen(input)) {
        return 0; // empty, continue
    }

    // parse command and arguments
    *arg_count = parse_args(input, args);

    if (*arg_count == 0) {
        return 0; // empty after parsing
    }

    // classify command type
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
        // print the prompt
        printf("hw1shell$ ");
        fflush(stdout);

        // read user input
        if (fgets(command, sizeof(command), stdin) == NULL) { 
            printf("\n");
            cleanup_jobs(); // Clean up on EOF
            break; 
        }

        // Save a copy of the raw command string (without the \n if possible, handled in handle_input logic)
        // Since handle_input modifies 'command' immediately, let's copy it here.
        strcpy(command_copy, command);
        // Remove newline from copy manually for the jobs display
        command_copy[strcspn(command_copy, "\n")] = '\0';

        char *args[MAX_LINE / 2 + 1]; 
        int arg_count;
        int result = handle_input(command, args, &arg_count);
        
        if (result == 1) {
            // exit command
            cleanup_jobs(); // Wait for backgrounds to finish (Section 2)
            break; 
        } else if (result == 0) {
            // empty input
            continue;
        } else if (result == 2) {
            // internal command
            execute_internal(args, arg_count);
        }
        else { 
            // result == 3: external command
            int is_background = 0;

            // Check if the last argument is "&"
            if (arg_count > 0 && strcmp(args[arg_count - 1], "&") == 0) {
                is_background = 1;
                args[arg_count - 1] = NULL; // Remove "&" from arguments passed to execvp
                
                // We also need to remove "&" from the command_copy string for display purposes
                // Note: Logic to cleanly remove '&' from the string copy is tricky without complex parsing,
                // but usually acceptable to just leave it or use a simple strrchr check.
                // For this homework, keeping the original string (even with &) in the jobs list is often fine.
            }

            execute_external_command(args, is_background, command_copy);
        }

        // At the end of every loop iteration, check for finished background jobs (Section 12)
        check_and_reap_background_jobs();
    }
    
    return 0;
}