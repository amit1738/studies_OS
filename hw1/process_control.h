#ifndef PROCESS_CONTROL_H
#define PROCESS_CONTROL_H

#include <sys/types.h>

#define MAX_LINE 1024
#define MAX_BACKGROUND_JOBS 4

// Job structure to track background processes
typedef struct {
    pid_t pid;
    char *command_line; // Original command string
} Job;

// Execute an external command (fork + execvp)
// Handles both foreground (wait) and background (add to jobs list)
void execute_external_command(char **args, int is_background, const char *original_line);

// Check if any background processes have finished (zombies) and reap them
void check_and_reap_background_jobs();

// Print the list of current background jobs (for 'jobs' command)
void print_jobs();

// Wait for all background jobs to finish and free memory (for 'exit')
void cleanup_jobs();

#endif // PROCESS_CONTROL_H