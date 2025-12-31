#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

#define MAX_BUFFER 2048
#define MAX_NAME_LEN 64
#define MAX_CLIENTS 100

// Helper to handle errors nicely
void error(const char *msg) {
    perror(msg);
    exit(1);
}

#endif