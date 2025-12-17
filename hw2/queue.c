#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

// Enqueue a new job to the global queue
// Assumes the caller holds the queue_lock!
void enqueue(char *line, long long read_time) {
    Job *new_job = (Job *)malloc(sizeof(Job));
    if (new_job == NULL) {
        perror("Failed to allocate memory for new job");
        exit(EXIT_FAILURE);
    }
    new_job->cmd_line = line;
    new_job->read_time = read_time;
    new_job->next = NULL;

    if (job_queue_tail == NULL) {
        job_queue_head = new_job;
        job_queue_tail = new_job;
    } else {
        job_queue_tail->next = new_job;
        job_queue_tail = new_job;
    }
}
