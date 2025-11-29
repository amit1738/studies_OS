#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h> //its allowed?

#include "error_handling.h"
#include "internal.h"

//TODO: define all constraints from requirements. like args
#define MAX_LINE 1024 


//parse command into arguments
//returns: number of arguments parsed
int parse_args(char *input, char **args) {
    int arg_count = 0;
    char *token = strtok(input, " \t"); //tokenize by space and tab
    while (token != NULL && arg_count < MAX_LINE / 2) {
        args[arg_count] = token;
        arg_count++;
        token = strtok(NULL, " \t");
    }
    args[arg_count] = NULL;
    return arg_count;
}



//handling input function
//returns: 0 = empty, 1 = exit command, 2 = internal command, 3 = external command
int handle_input(char *input, char **args, int *arg_count) {
    
    //remove newline suffix
    input[strcspn(input, "\n")] = '\0';

    //check for empty input - empty or whitespace only
    if (input[0] == '\0' || strspn(input, " \t") == strlen(input)) {
        return 0; //empty, continue
    }

    //parse command and arguments
    *arg_count = parse_args(input, args);

    if (*arg_count == 0) {
        return 0; //empty after parsing
    }

    //classify command type
    if (strcmp(args[0], "exit") == 0) {
        return 1; //exit command
    }
    
    if (strcmp(args[0], "cd") == 0 || strcmp(args[0], "jobs") == 0) {
        return 2; //internal command
    }

    //external command 
    //TODO: check if valid external command? and handle errors?
    return 3;
}

int main(void) {
    char command[MAX_LINE];
    
    while(1) {
        //print the prompt
        printf("hw1shell$ ");
        fflush(stdout);

        //read user input
        if (fgets(command, sizeof(command), stdin) == NULL) { //TODO handle EOF properly and understand fgets retunn value
            break; // Exit on EOF or error 
        }

        char *args[MAX_LINE / 2 + 1]; //max args is half of MAX_LINE because of spaces
        int arg_count;
        int result = handle_input(command, args, &arg_count);
        
        if (result == 1) {
            //exit command
            break; //TODO: handle cleanup if needed
        } else if (result == 0) {
            //empty input
            continue;
        } else if (result == 2) {
            //internal command - execute it
            execute_internal(args, arg_count);
            continue;
        }
        else { //result == 3: external command - execute with fork/exec
            if (fork() == 0) {
                //child process
                if (execvp(args[0], args) == -1) { //TODO: when redy will change to external execution function wirh process management
                    //if execvp fails
                    print_error_systemcall(args[0], errno); //TODO: mabey implement inside external execution function
            } else {
                //parent process
                wait(NULL); //wait for child to finish
            }
        }

    }
    
    return 0;
}