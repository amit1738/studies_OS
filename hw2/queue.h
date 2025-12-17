#ifndef QUEUE_H
#define QUEUE_H

#include "worker.h"

// We use the Job struct from worker.h as the node
// No separate Queue struct needed if we use global head/tail, 
// but we can keep helper functions.

void enqueue(char *line, long long read_time);

#endif
