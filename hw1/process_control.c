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
            background_jobs[i].command_line = strdup(command_line); // Save copy of command
            if (background_jobs[i].command_line == NULL) {
                // print_error_systemcall("strdup", errno);
                background_jobs[i].pid = 0; // Cancel the slot reservation
                return; // Return early on error
            }
            active_jobs_count++;
            return;
        }
    }
    // Implicit return here if loop finishes (should never happen)
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
    } 
    else if (pid == 0) {
        // --- Child Process ---
        if (execvp(args[0], args) == -1) {
            print_error_systemcall("execvp", errno);
            // Guidelines say: display invalid command message
            printf("hw1shell: invalid command\n"); 
            exit(1);
        }
    } 
    else {
        // --- Parent Process ---
        if (is_background) {
            // Background: Record job and continue (Section 9)
            printf("hw1shell: pid %d started\n", pid);
            add_job(pid, original_line);
        } else {
            // Foreground: Wait for this specific child (Section 8)
            if (waitpid(pid, NULL, 0) == -1) {
                // Ignore interruption by signal, report other errors
                if (errno != EINTR) {
                    print_error_systemcall("waitpid", errno);
                }
            }
        }
    }
}

void check_and_reap_background_jobs() {
    pid_t pid;
    int status;
    
    // Check all slots for finished processes (Section 12)
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].pid > 0) {
            // WNOHANG is critical: don't block if child is still running
            pid = waitpid(background_jobs[i].pid, &status, WNOHANG);
            
            if (pid > 0) {
                // Child finished
                printf("hw1shell: pid %d finished\n", pid);
                remove_job(pid);
            }
            else if (pid == -1 && errno != ECHILD) {
                 print_error_systemcall("waitpid", errno);
            }
        }
    }
}

void print_jobs() {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].pid != 0) {
            // Format: PID \t Command (Section 4)
            printf("%d\t%s\n", background_jobs[i].pid, background_jobs[i].command_line);
        }
    }
}

void cleanup_jobs() {
    // Used on exit: wait for all background jobs (Section 2)
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].pid != 0) {
            waitpid(background_jobs[i].pid, NULL, 0);
            remove_job(background_jobs[i].pid);
        }
    }
}