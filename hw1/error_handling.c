#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>


//print error
void print_error_systemcall(char syscall_name, int errnum) {
    printf("hw1shell: %s failed. errno is %d\n", syscall_name, errnum);
}

