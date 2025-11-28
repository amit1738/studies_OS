#include <stdio.h>
#include <string.h>
#include <errno.h>

#define MAX_LINE 1024

int main(void) {
    char command[MAX_LINE];
    
    while(1) {
        //print the prompt
        printf("hw1shell$ ");
        fflush(stdout);

        //read user input
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break; // Exit on EOF or error
        }

        //remove newline
        command[strcspn(command, "\n")] = '\0';

        //exit condition
        if (strcmp(command, "exit") == 0) {
            break;
        }

        //execute command
        if (fork() == 0) {
            //child process
            execlp(command, command, (char *)NULL);
            //if execlp fails
            fprintf(stderr, "Error executing command '%s': %s\n", command, strerror(errno));
            return 1;
        } else {
            //parent process
            wait(NULL); //wait for child to finish
        }

    }
    
    return 0;
}
    