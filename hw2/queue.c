/*
 * queue.c - Simple linked list job queue
 * Note: caller must hold queue_lock when calling these functions
 */

#include <stdlib.h>
#include "queue.h"

void enqueue(char *cmd, long long submit_time) {
    Job *j = malloc(sizeof(Job));
    j->cmd = cmd;
    j->submit_time = submit_time;
    j->next = NULL;

    if (queue_tail == NULL) {
        queue_head = queue_tail = j;
    } else {
        queue_tail->next = j;
        queue_tail = j;
    }
}

Job *dequeue(void) {
    if (queue_head == NULL)
        return NULL;

    Job *j = queue_head;
    queue_head = queue_head->next;
    if (queue_head == NULL)
        queue_tail = NULL;

    return j;
}
