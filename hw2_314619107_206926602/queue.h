/*
 * queue.h - Job queue interface
 */

#ifndef QUEUE_H
#define QUEUE_H

#include "worker.h"

void enqueue(char *cmd, long long submit_time);
Job *dequeue(void);

#endif
