#ifndef QUEUE_H
#define QUEUE_H

typedef struct Node {
    char *line;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
} Queue;

void init_queue(Queue *q);
void enqueue(Queue *q, char *line);
char *dequeue(Queue *q); // Adding dequeue for future use by workers

#endif
