#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "error_handling.h"
#include "process_control.h" // Added include

// Execute built-in shell commands (cd, jobs)
// Returns 0 on success, -1 on error
int execute_internal(char **args, int arg_count) {
    // Handle cd command
    if (strcmp(args[0], "cd") == 0) {
        if (arg_count < 2) {
            printf("hw1shell: invalid command\n");
            return -1;
        }
        
        // Try to change directory
        if (chdir(args[1]) != 0) {
            print_error_systemcall("chdir", errno);
            return -1;
        }
        return 0;
    }
    
    // Handle jobs command - display all running background processes
    if (strcmp(args[0], "jobs") == 0) {
        extern Job background_jobs[MAX_BACKGROUND_JOBS];
        
        for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
            if (background_jobs[i].pid != 0) {
                printf("%d\t%s\n", background_jobs[i].pid, background_jobs[i].command_line);
            }
        }
        return 0;
    }
    
    return -1;
}