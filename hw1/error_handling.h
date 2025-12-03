#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

// Display error message for failed system calls
void print_error_systemcall(char *syscall_name, int errnum);

#endif // ERROR_HANDLING_H