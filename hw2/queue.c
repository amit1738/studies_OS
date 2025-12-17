#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

void init_queue(Queue *q) {
    q->head = NULL;
    q->tail = NULL;
}

void enqueue(Queue *q, char *line) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("Failed to allocate memory for new node");
        exit(EXIT_FAILURE);
    }
    new_node->line = line;
    new_node->next = NULL;

    if (q->tail == NULL) {
        q->head = new_node;
        q->tail = new_node;
    } else {
        q->tail->next = new_node;
        q->tail = new_node;
    }
}

char *dequeue(Queue *q) {
    if (q->head == NULL) {
        return NULL;
    }
    Node *temp = q->head;
    char *line = temp->line;
    q->head = q->head->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    free(temp);
    return line;
}
