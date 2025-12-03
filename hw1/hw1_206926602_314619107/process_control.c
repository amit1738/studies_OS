#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "process_control.h"
#include "error_handling.h" // Use your partner's error function

// Global array to track jobs
Job background_jobs[MAX_BACKGROUND_JOBS];
int active_jobs_count = 0;

// Helper: Find a free slot in the jobs array
void add_job(pid_t pid, const char *command_line) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].pid == 0) {
            background_jobs[i].pid = pid;
            background_jobs[i].command_line = strdup(command_line); // Save copy of command for print in "jobs" command.
            if (background_jobs[i].command_line == NULL) {
                background_jobs[i].pid = 0;
                return;
            }
            active_jobs_count++;
            return;
        }
    }
}

// Helper: Remove a job from the list
void remove_job(pid_t pid) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].pid == pid) {
            background_jobs[i].pid = 0;
            if (background_jobs[i].command_line) {
                free(background_jobs[i].command_line);
                background_jobs[i].command_line = NULL;
            }
            active_jobs_count--;
            return;
        }
    }
}

void execute_external_command(char **args, int is_background, const char *original_line) {
    // Check constraints (Section 6)
    if (is_background && active_jobs_count >= MAX_BACKGROUND_JOBS) {
        printf("hw1shell: too many background commands running\n");
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        print_error_systemcall("fork", errno);
        return;
    } 
    else if (pid == 0) {
        // --- Child Process ---
        execvp(args[0], args);
        // Only reached if execvp fails
        print_error_systemcall(args[0], errno);
        printf("hw1shell: invalid command\n"); 
        exit(1);
    } 
    else {
        // Parent process
        if (is_background) {
            // Background job: record it and continue
            printf("hw1shell: pid %d started\n", pid);
            add_job(pid, original_line);
        } else {
            // Foreground job: wait for it to complete
            if (waitpid(pid, NULL, 0) == -1) {
                if (errno != EINTR) {
                    print_error_systemcall("waitpid", errno);
                }
            }
        }
    }
}

void check_and_reap_background_jobs() {
    pid_t pid;
    
    // Check each background job to see if it finished
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].pid > 0) {
            // Use WNOHANG to check without blocking
            pid = waitpid(background_jobs[i].pid, NULL, WNOHANG);
            
            if (pid > 0) {
                // Process finished - reap it and free the slot
                printf("hw1shell: pid %d finished\n", pid);
                remove_job(pid);
            }
            else if (pid == -1 && errno != ECHILD) {
                 print_error_systemcall("waitpid", errno);
            }
        }
    }
}



void cleanup_jobs() {
    // Wait for all background jobs before shell exits
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].pid != 0) {
            waitpid(background_jobs[i].pid, NULL, 0);
            remove_job(background_jobs[i].pid);
        }
    }
}