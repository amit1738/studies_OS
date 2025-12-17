#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>
#include <stdio.h>

/* --- Data Structures --- */

// The Job structure for the linked list queue
typedef struct Job {
    char *cmd_line;       // The full command string (e.g., "msleep 10; increment 5")
    long long read_time;  // Time (ms) when dispatcher read this line (for stats)
    struct Job *next;     // Pointer to the next job in the queue
} Job;

/* --- Shared Variables (Must be defined in main.c) --- */

// Queue Synchronization
extern pthread_mutex_t queue_lock; // before worker checks the queue, he needs to lock this so other workers/dispatcher won't be able to modify the queue.
extern pthread_cond_t queue_cond; // if the queue 
extern Job *job_queue_head;   // Head of the linked list
extern Job *job_queue_tail;   // Tail of the linked list (added for O(1) enqueue)
extern int dispatcher_done;   // Flag: 0 = running, 1 = dispatcher finished reading and we can exit the workers(threads)
extern int active_jobs;       // Count of jobs currently in queue or processing

// Statistics Synchronization
extern pthread_mutex_t stats_lock;
extern long long sum_turnaround; // Sum of all job turnaround times
extern long long min_turnaround;
extern long long max_turnaround;
extern long long total_jobs_done;

// File Handling & Config
extern pthread_mutex_t file_locks[]; // Array of mutexes (one per counter file)
extern int log_enabled;              // 1 = write logs, 0 = silent

/* --- Function Prototypes --- */

// Initializes worker threads
void init_workers(int num_threads);

// Waits for all worker threads to finish and cleans up
void join_workers(int num_threads);

// The main loop for the worker thread (internal use, but needed for pthread_create)
void* worker_function(void* arg);

#endif
