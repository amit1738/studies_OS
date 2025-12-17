#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "queue.h"

#define num_threads 4096
#define num_counters 100
#define log_enabled

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t all_done = PTHREAD_COND_INITIALIZER;
int active_jobs = 0;

void init_counter_files() {
    for (int i = 0; i < num_counters; i++) {
        char filename[11];
        sprintf(filename, "count%02d.txt", i);
        FILE *file = fopen(filename, "w");
        if (file != NULL) {
            fclose(file);
        }
    }
}

void read_instruction_file(const char *filename, Queue *fifo) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline character if present
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        // Skip empty lines
        if (len == 0) continue;

        char cmd_type[32];
        char cmd_arg[32];
        int ms_arg;

        // Try to parse "worker <args>"
        if (strncmp(line, "worker", 6) == 0) {
            pthread_mutex_lock(&lock);
            // Skip "worker" and following whitespace
            char *args = line + 6;
            while (*args == ' ' || *args == '\t') args++;
            enqueue(fifo, strdup(args));
            active_jobs++;
            pthread_mutex_unlock(&lock);
        } 
        // Try to parse "dispatcher_<cmd> <arg>"
        else if (sscanf(line, "%s %d", cmd_type, &ms_arg) >= 1) {
            if (strcmp(cmd_type, "dispatcher_msleep") == 0) {
                usleep(ms_arg * 1000);
            } else if (strcmp(cmd_type, "dispatcher_wait") == 0) {
                pthread_mutex_lock(&lock);
                while (active_jobs > 0) {
                    pthread_cond_wait(&all_done, &lock);
                }
                pthread_mutex_unlock(&lock);
            }
        }
    }

    fclose(file);
}

void main() {
    Queue fifo;
    init_queue(&fifo);
}
