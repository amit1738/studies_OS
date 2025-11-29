#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>


//execute internal commands (exit, cd, jobs)
//returns: 0 = success, -1 = error
int execute_internal(char **args, int arg_count) {
    if (strcmp(args[0], "cd") == 0) {
        if (arg_count < 2) {
            fprintf(stderr, "cd: missing argument\n");
            return -1;
        } else if (chdir(args[1]) != 0) {
            fprintf(stderr, "cd: %s: %s\n", args[1], strerror(errno)); //TODO: need to understand
            return -1;
        }
        return 0;
    }
    
    if (strcmp(args[0], "jobs") == 0) {
        //TODO: implement jobs listing
        printf("jobs: not yet implemented\n");
        return 0;
    }
    
    return -1; //should not reach here
}