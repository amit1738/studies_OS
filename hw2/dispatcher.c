#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include "queue.h"
#include "worker.h"

#define MAX_CMD_LEN 1024


// --- Global Variables Definition ---
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t completion_cond = PTHREAD_COND_INITIALIZER;
Job *job_queue_head = NULL;
Job *job_queue_tail = NULL;
int dispatcher_done = 0;
int active_jobs = 0; //fixme

pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;
long long sum_turnaround = 0;
long long min_turnaround = 0;
long long max_turnaround = 0;
long long total_jobs_done = 0;

pthread_mutex_t file_locks[100];
int log_enabled = 0;
long long program_start_time = 0;

// --- Helper Functions ---

long long get_time_ms_dispatcher() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long current = (long long)(tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
    return current - program_start_time;
}

void init_counter_files(int num_counters) {
    for (int i = 0; i < num_counters; i++) {
        char filename[32];
        sprintf(filename, "count%02d.txt", i);
        FILE *file = fopen(filename, "w");
        if (file != NULL) {
            fprintf(file, "0");
            fclose(file);
        }
        pthread_mutex_init(&file_locks[i], NULL); //fixme
    }
}

void read_instruction_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening command file");
        exit(EXIT_FAILURE);
    }

    FILE *dispatcher_log = NULL;
    if (log_enabled) {
        dispatcher_log = fopen("dispatcher.txt", "w");
    }

    char line[MAX_CMD_LEN];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        if (len == 0) continue;

        // Log read command
        if (dispatcher_log) {
            fprintf(dispatcher_log, "TIME %lld: read cmd line: %s\n", get_time_ms_dispatcher(), line);
            fflush(dispatcher_log);
        }

        char cmd_type[32];
        int ms_arg;

        // Pointer for parsing that skips leading whitespace, without altering original line
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;

        // 1. Worker Job
        if (strncmp(p, "worker", 6) == 0) {
            // Skip "worker" and whitespace
            char *args = p + 6;
            while (*args == ' ' || *args == '\t') args++;
            
            pthread_mutex_lock(&queue_lock);
            enqueue(strdup(args), get_time_ms_dispatcher());
            active_jobs++;
            pthread_cond_signal(&queue_cond); // Wake up a worker
            pthread_mutex_unlock(&queue_lock);
        } 
        // 2. Dispatcher Command
        else if (sscanf(p, "%s %d", cmd_type, &ms_arg) >= 1) {
            if (strcmp(cmd_type, "dispatcher_msleep") == 0) {
                usleep(ms_arg * 1000);
            } else if (strcmp(cmd_type, "dispatcher_wait") == 0) {
                pthread_mutex_lock(&queue_lock);
                while (active_jobs > 0) {
                    pthread_cond_wait(&completion_cond, &queue_lock);
                }
                pthread_mutex_unlock(&queue_lock);
            }
        }
    }

    if (dispatcher_log) fclose(dispatcher_log);
    fclose(file);
}

void write_stats(long long total_runtime) {
    FILE *f = fopen("stats.txt", "w");
    if (!f) {
        perror("Error opening stats.txt");
        return;
    }

    double avg_turnaround = 0;
    if (total_jobs_done > 0) {
        avg_turnaround = (double)sum_turnaround / total_jobs_done;
    }

    fprintf(f, "total running time: %lld milliseconds\n", total_runtime);
    fprintf(f, "sum of jobs turnaround time: %lld milliseconds\n", sum_turnaround);
    fprintf(f, "min job turnaround time: %lld milliseconds\n", min_turnaround);
    fprintf(f, "average job turnaround time: %f milliseconds\n", avg_turnaround);
    fprintf(f, "max job turnaround time: %lld milliseconds\n", max_turnaround);

    fclose(f);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <cmdfile> <num_threads> <num_counters> <log_enabled>\n", argv[0]);
        return 1;
    }

    char *cmdfile = argv[1];
    int num_threads = atoi(argv[2]);
    int num_counters = atoi(argv[3]);
    log_enabled = atoi(argv[4]);

    if (num_threads > 4096) num_threads = 4096;
    if (num_counters > 100) num_counters = 100;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    program_start_time = (long long)(tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
    
    long long start_time = get_time_ms_dispatcher();

    // Initialize
    init_counter_files(num_counters);
    init_workers(num_threads);

    // Process commands
    read_instruction_file(cmdfile);

    // Wait for all jobs to finish
    pthread_mutex_lock(&queue_lock);
    while (active_jobs > 0) {
        pthread_cond_wait(&completion_cond, &queue_lock);
    }
    
    // Signal workers to exit
    dispatcher_done = 1;
    pthread_cond_broadcast(&queue_cond);
    pthread_mutex_unlock(&queue_lock);

    // Join workers
    join_workers(num_threads);

    long long end_time = get_time_ms_dispatcher();
    write_stats(end_time - start_time);

    return 0;
}
