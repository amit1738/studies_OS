#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define num_threads 4096
#define num_counters 100
#define log_enabled


void init_counter_files() {
    for (int i = 0; i < num_counters; i++) {
        char filename[11];
        sprintf(filename, "count%02d.txt", i);
        FILE *file = fopen(filename, "w");
        fwrite(";
        fclose(file);


void main() {}
