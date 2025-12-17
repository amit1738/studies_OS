#include <worker.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_CMD_LEN 1024
#define MAX_SUB_CMDS 100 // Max number of basic commands in one line

// Internal global arrays to manage thread handles
pthread_t *worker_handles = NULL;
int *worker_indices = NULL;

/* --- Helper Functions --- */

// Get current wall time in milliseconds
long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
}

// Thread-safe update of counter files (counters are 0-99)
void update_counter_file(int counter_id, int change) {
    if (counter_id < 0 || counter_id >= 100) return; 

    char filename[32];
    sprintf(filename, "count%02d.txt", counter_id);

    // CRITICAL SECTION: File Update
    // We lock only the specific mutex for this file ID
    pthread_mutex_lock(&file_locks[counter_id]);

    FILE *f = fopen(filename, "r+"); // Open for reading and updating
    if (f) {
        long long val = 0;
        if (fscanf(f, "%lld", &val) == 1) {
            val += change;
            rewind(f); // Move cursor back to start
            fprintf(f, "%lld", val);
        }
        fclose(f);
    } else {
        // Fallback if file doesn't exist (should be created by dispatcher)
        perror("Error opening counter file");
    }

    pthread_mutex_unlock(&file_locks[counter_id]);
}

// Parse and execute a single basic command (e.g., "msleep 50")
void execute_basic_cmd(char *cmd) {
    char *saveptr;
    // Work on a copy because strtok modifies the string
    char cmd_copy[MAX_CMD_LEN];
    strncpy(cmd_copy, cmd, MAX_CMD_LEN);

    // Parse: type argument (e.g., "increment" "5")
    char *type = strtok_r(cmd_copy, " ", &saveptr);
    char *arg_str = strtok_r(NULL, " ", &saveptr);

    if (!type || !arg_str) return;

    int arg = atoi(arg_str);

    if (strcmp(type, "msleep") == 0) {
        usleep(arg * 1000); // usleep takes microseconds, input is ms
    } else if (strcmp(type, "increment") == 0) {
        update_counter_file(arg, 1); //
    } else if (strcmp(type, "decrement") == 0) {
        update_counter_file(arg, -1); //
    }
}

// Handle the full command line including the 'repeat' logic
void process_job_line(char *line) {
    // We must tokenize by ';' first to handle "repeat" correctly.
    // "repeat x" applies to the SEQUENCE starting after it.
    
    char *commands[MAX_SUB_CMDS]; 
    int cmd_count = 0;
    char *saveptr;
    
    // Copy original line to preserve it for logging
    char line_copy[MAX_CMD_LEN];
    strncpy(line_copy, line, MAX_CMD_LEN);

    // Split by semicolon
    char *token = strtok_r(line_copy, ";", &saveptr);
    while (token != NULL && cmd_count < MAX_SUB_CMDS) {
        // Skip leading spaces for cleaner parsing
        while (*token == ' ') token++;
        commands[cmd_count++] = token;
        token = strtok_r(NULL, ";", &saveptr);
    }

    // Iterate through commands
    for (int i = 0; i < cmd_count; i++) {
        // Check if this command is "repeat"
        if (strncmp(commands[i], "repeat", 6) == 0) {
            int repeat_times = 0;
            sscanf(commands[i], "repeat %d", &repeat_times);
            
            // repeat x times the entire sequence AFTER the repeat command
            for (int r = 0; r < repeat_times; r++) {
                for (int j = i + 1; j < cmd_count; j++) {
                    execute_basic_cmd(commands[j]);
                }
            }
            break; // Stop outer loop; the rest of the line was handled by the repeat logic
        } else {
            // Normal execution
            execute_basic_cmd(commands[i]);
        }
    }
}

/* --- Worker Thread Logic --- */

void* worker_function(void* arg) {
    int thread_id = *((int*)arg);
    
    // Open log file: threadxx.txt
    char log_name[32];
    sprintf(log_name, "thread%02d.txt", thread_id);
    FILE *logfile = NULL;
    
    if (log_enabled) {
        logfile = fopen(log_name, "w");
    }

    while (1) {
        Job *job = NULL;

        // 1. Lock Queue
        pthread_mutex_lock(&queue_lock);
        
        // 2. Wait while queue is empty AND dispatcher is NOT done
        while (job_queue_head == NULL && !dispatcher_done) {
            pthread_cond_wait(&queue_cond, &queue_lock);
        }

        // 3. Exit Condition: Queue empty AND dispatcher is done
        if (job_queue_head == NULL && dispatcher_done) {
            pthread_mutex_unlock(&queue_lock);
            break; 
        }

        // 4. Pop Job
        job = job_queue_head;
        job_queue_head = job->next;
        
        pthread_mutex_unlock(&queue_lock); // Release lock ASAP

        // 5. Process Job
        if (job) {
            long long start_time = get_time_ms();

            // Log START
            if (logfile) {
                fprintf(logfile, "TIME %lld: START job %s\n", start_time, job->cmd_line);
                fflush(logfile); // Ensure write
            }

            process_job_line(job->cmd_line);

            long long end_time = get_time_ms();

            // Log END
            if (logfile) {
                fprintf(logfile, "TIME %lld: END job %s\n", end_time, job->cmd_line);
                fflush(logfile);
            }

            // 6. Update Stats
            long long turnaround = end_time - job->read_time;
            
            pthread_mutex_lock(&stats_lock);
            sum_turnaround += turnaround;
            if (total_jobs_done == 0 || turnaround < min_turnaround) {
                min_turnaround = turnaround;
            }
            if (turnaround > max_turnaround) {
                max_turnaround = turnaround;
            }
            total_jobs_done++;
            pthread_mutex_unlock(&stats_lock);

            // 7. Cleanup Job Memory
            free(job->cmd_line);
            free(job);
        }
    }

    if (logfile) fclose(logfile);
    return NULL;
}

/* --- Initialization & Cleanup --- */

void init_workers(int num_threads) {
    // Allocate arrays to store thread handles and IDs
    worker_handles = (pthread_t*) malloc(num_threads * sizeof(pthread_t));