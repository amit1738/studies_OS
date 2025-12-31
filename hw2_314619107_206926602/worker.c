/*
 * worker.c - Worker thread implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "worker.h"
#include "queue.h"

#define MAX_LINE 1024

static pthread_t *threads = NULL;
static int *thread_ids = NULL;

/* Update a counter file by adding delta (+1 or -1) */
void update_counter(int id, int delta) {
    if (id < 0 || id >= 100) return;

    char fname[32];
    sprintf(fname, "count%02d.txt", id);

    pthread_mutex_lock(&counter_locks[id]);

    FILE *f = fopen(fname, "r+");
    if (f) {
        long long val = 0;
        fscanf(f, "%lld", &val);
        val += delta;
        rewind(f);
        fprintf(f, "%lld", val);
        fflush(f);
        ftruncate(fileno(f), ftell(f));  // in case new value is shorter
        fclose(f);
    }

    pthread_mutex_unlock(&counter_locks[id]);
}

/* Execute a single basic command like "increment 5" or "msleep 100" */
void run_cmd(char *cmd) {
    char buf[MAX_LINE];
    strncpy(buf, cmd, MAX_LINE);
    buf[MAX_LINE-1] = '\0';

    char *saveptr;
    char *op = strtok_r(buf, " \t", &saveptr);
    char *arg = strtok_r(NULL, " \t", &saveptr);

    if (!op) return;

    int val = 0;
    if (arg)
        val = atoi(arg);

    if (strcmp(op, "msleep") == 0 && val > 0) {
        usleep(val * 1000);
    }
    else if (strcmp(op, "increment") == 0 && arg) {
        update_counter(val, 1);
    }
    else if (strcmp(op, "decrement") == 0 && arg) {
        update_counter(val, -1);
    }
    // unknown commands are ignored
}

/* Process entire job string, handling repeat and semicolons */
void process_job(char *job) {
    char buf[MAX_LINE];
    strncpy(buf, job, MAX_LINE);
    buf[MAX_LINE-1] = '\0';

    // split by semicolon
    char *cmds[100];
    int n = 0;
    char *saveptr;
    char *tok = strtok_r(buf, ";", &saveptr);
    while (tok && n < 100) {
        while (*tok && (*tok == ' ' || *tok == '\t')) tok++;  // trim
        cmds[n++] = tok;
        tok = strtok_r(NULL, ";", &saveptr);
    }

    for (int i = 0; i < n; i++) {
        if (strncmp(cmds[i], "repeat", 6) == 0) {
            int times = 0;
            sscanf(cmds[i] + 6, "%d", &times);
            // repeat everything after this command
            for (int r = 0; r < times; r++) {
                for (int j = i + 1; j < n; j++) {
                    run_cmd(cmds[j]);
                }
            }
            break;  // done after repeat
        }
        run_cmd(cmds[i]);
    }
}

/* Worker thread main loop */
void *worker_main(void *arg) {
    int id = *(int *)arg;

    FILE *log = NULL;
    if (log_enabled) {
        char fname[32];
        sprintf(fname, "thread%02d.txt", id);
        log = fopen(fname, "w");
    }

    while (1) {
        pthread_mutex_lock(&queue_lock);

        // wait for work or shutdown
        while (queue_head == NULL && !done)
            pthread_cond_wait(&queue_cond, &queue_lock);

        if (queue_head == NULL && done) {
            pthread_mutex_unlock(&queue_lock);
            break;
        }

        // grab job from queue
        Job *job = dequeue();
        pthread_mutex_unlock(&queue_lock);

        if (!job) continue;

        long long t_start = get_time_ms();
        if (log) {
            fprintf(log, "TIME %lld: START job %s\n", t_start, job->cmd);
            fflush(log);
        }

        process_job(job->cmd);

        long long t_end = get_time_ms();
        if (log) {
            fprintf(log, "TIME %lld: END job %s\n", t_end, job->cmd);
            fflush(log);
        }

        // update stats
        long long turnaround = t_end - job->submit_time;
        pthread_mutex_lock(&stats_lock);
        stats_sum += turnaround;
        if (stats_count == 0 || turnaround < stats_min) stats_min = turnaround;
        if (turnaround > stats_max) stats_max = turnaround;
        stats_count++;
        pthread_mutex_unlock(&stats_lock);

        // signal dispatcher that we finished a job
        pthread_mutex_lock(&queue_lock);
        active_jobs--;
        if (active_jobs == 0)
            pthread_cond_signal(&done_cond);
        pthread_mutex_unlock(&queue_lock);

        free(job->cmd);
        free(job);
    }

    if (log) fclose(log);
    return NULL;
}

void init_workers(int n) {
    threads = malloc(n * sizeof(pthread_t));
    thread_ids = malloc(n * sizeof(int));

    for (int i = 0; i < n; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, worker_main, &thread_ids[i]);
    }
}

void join_workers(int n) {
    for (int i = 0; i < n; i++)
        pthread_join(threads[i], NULL);
    free(threads);
    free(thread_ids);
}
