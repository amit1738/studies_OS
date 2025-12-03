#ifndef PROCESS_CONTROL_H
#define PROCESS_CONTROL_H

#include <sys/types.h>

#define MAX_LINE 1024
#define MAX_BACKGROUND_JOBS 4

// Structure to track a background job
typedef struct {
    pid_t pid;
    char *command_line;
} Job;

// Fork and execute an external command
void execute_external_command(char **args, int is_background, const char *original_line);

// Check for finished background jobs and reap them
void check_and_reap_background_jobs();

// Wait for all background jobs before exiting
void cleanup_jobs();

// Global jobs array accessible to other modules
extern Job background_jobs[MAX_BACKGROUND_JOBS];

#endif // PROCESS_CONTROL_H