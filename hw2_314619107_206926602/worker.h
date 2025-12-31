/*
 * worker.h - Worker thread interface
 */

#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>

/* Job node for the queue */
typedef struct Job {
    char *cmd;              // command string (everything after "worker ")
    long long submit_time;  // when dispatcher read this line
    struct Job *next;
} Job;

/* Shared globals - defined in dispatcher.c */
extern pthread_mutex_t queue_lock;
extern pthread_cond_t queue_cond;
extern pthread_cond_t done_cond;
extern Job *queue_head;
extern Job *queue_tail;
extern int done;
extern int active_jobs;

extern pthread_mutex_t stats_lock;
extern long long stats_sum;
extern long long stats_min;
extern long long stats_max;
extern long long stats_count;

extern pthread_mutex_t counter_locks[];
extern int log_enabled;
extern long long start_time;

/* Functions */
long long get_time_ms(void);
void init_workers(int n);
void join_workers(int n);

#endif
