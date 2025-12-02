#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "error_handling.h"
#include "process_control.h" // Added include

// execute internal commands (exit, cd, jobs)
// returns: 0 = success, -1 = error
int execute_internal(char **args, int arg_count) {
    // cd command
    if (strcmp(args[0], "cd") == 0) {
        if (arg_count < 2) {
            printf("hw1shell: invalid command\n");
            return -1;
        } else if (strcmp(args[1], "..") == 0) {    
            // go up one directory
            if (chdir("..") != 0) {
                print_error_systemcall("chdir", errno);
                return -1;
            }
        } else {
            // change to specified directory
            if (chdir(args[1]) != 0) {
                print_error_systemcall("chdir", errno);
                return -1;
            }
        }
        return 0;
    }
    
    // jobs command implementation
    if (strcmp(args[0], "jobs") == 0) {
        print_jobs(); // Call the function from process_control.c
        return 0;
    }
    
    return -1; // should not reach here
}