/*
 * dispatcher.c - Main dispatcher for HW2
 * Reads commands from file and dispatches jobs to worker threads
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <ctype.h>
#include "queue.h"
#include "worker.h"

#define MAX_LINE 1024

/* Global synchronization */
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;   // wake workers when job available
pthread_cond_t done_cond = PTHREAD_COND_INITIALIZER;    // wake dispatcher when job done

Job *queue_head = NULL;
Job *queue_tail = NULL;
int done = 0;           // 1 when dispatcher finished reading file
int active_jobs = 0;    // jobs currently in queue or being processed

/* Stats tracking */
pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;
long long stats_sum = 0;
long long stats_min = 0;
long long stats_max = 0;
long long stats_count = 0;

/* Per-counter locks (one mutex per counter file) */
pthread_mutex_t counter_locks[100];

/* Config */
int log_enabled = 0;
long long start_time = 0;

/* Returns milliseconds since program start */
long long get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000LL + tv.tv_usec / 1000) - start_time;
}

/* Initialize counter files to 0 */
void create_counters(int n) {
    char fname[32];
    for (int i = 0; i < n; i++) {
        sprintf(fname, "count%02d.txt", i);
        FILE *f = fopen(fname, "w");
        if (f) {
            fprintf(f, "0");
            fclose(f);
        }
        pthread_mutex_init(&counter_locks[i], NULL);
    }
}

/* Read and process command file */
void process_commands(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("fopen");
        exit(1);
    }

    FILE *log = NULL;
    if (log_enabled)
        log = fopen("dispatcher.txt", "w");

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        // remove newline
        int len = strlen(line);
        if (len > 0 && line[len-1] == '\n')
            line[len-1] = '\0';
        if (strlen(line) == 0)
            continue;

        if (log) {
            fprintf(log, "TIME %lld: read cmd line: %s\n", get_time_ms(), line);
            fflush(log);
        }

        // skip leading whitespace
        char *p = line;
        while (*p && isspace(*p)) p++;

        if (strncmp(p, "worker", 6) == 0) {
            // queue worker job
            char *cmd = p + 6;
            while (*cmd && isspace(*cmd)) cmd++;

            pthread_mutex_lock(&queue_lock);
            enqueue(strdup(cmd), get_time_ms());
            active_jobs++;
            pthread_cond_signal(&queue_cond);
            pthread_mutex_unlock(&queue_lock);
        }
        else if (strncmp(p, "dispatcher_msleep", 17) == 0) {
            int ms = 0;
            sscanf(p + 17, "%d", &ms);
            usleep(ms * 1000);
        }
        else if (strncmp(p, "dispatcher_wait", 15) == 0) {
            pthread_mutex_lock(&queue_lock);
            while (active_jobs > 0)
                pthread_cond_wait(&done_cond, &queue_lock);
            pthread_mutex_unlock(&queue_lock);
        }
    }

    if (log) fclose(log);
    fclose(f);
}

/* Write statistics file */
void write_stats(long long total_time) {
    FILE *f = fopen("stats.txt", "w");
    if (!f) return;

    double avg = (stats_count > 0) ? (double)stats_sum / stats_count : 0;

    fprintf(f, "total running time: %lld milliseconds\n", total_time);
    fprintf(f, "sum of jobs turnaround time: %lld milliseconds\n", stats_sum);
    fprintf(f, "min job turnaround time: %lld milliseconds\n", stats_min);
    fprintf(f, "average job turnaround time: %f milliseconds\n", avg);
    fprintf(f, "max job turnaround time: %lld milliseconds\n", stats_max);

    fclose(f);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s cmdfile num_threads num_counters log_enabled\n", argv[0]);
        return 1;
    }

    char *cmdfile = argv[1];
    int num_threads = atoi(argv[2]);
    int num_counters = atoi(argv[3]);
    log_enabled = atoi(argv[4]);

    // spec says max 4096 threads, 100 counters
    if (num_threads > 4096) num_threads = 4096;
    if (num_counters > 100) num_counters = 100;

    // record start time
    struct timeval tv;
    gettimeofday(&tv, NULL);
    start_time = tv.tv_sec * 1000LL + tv.tv_usec / 1000;

    create_counters(num_counters);
    init_workers(num_threads);
    process_commands(cmdfile);

    // wait for all jobs to complete before exiting
    pthread_mutex_lock(&queue_lock);
    while (active_jobs > 0)
        pthread_cond_wait(&done_cond, &queue_lock);
    done = 1;
    pthread_cond_broadcast(&queue_cond);  // wake workers so they can exit
    pthread_mutex_unlock(&queue_lock);

    join_workers(num_threads);
    write_stats(get_time_ms());

    return 0;
}
