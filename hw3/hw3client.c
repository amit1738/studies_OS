/*
 * hw3client.c - Chat Client
 * 
 * This client connects to the chat server and allows the user to:
 * - Send messages to all users (normal message)
 * - Send private messages (@name message)
 * - Exit the chat (!exit)
 * 
 * Uses select() to handle both keyboard input and server messages
 * simultaneously without blocking.
 * 
 * Usage: ./hw3client <server_ip> <port> <name>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

/* Constants as specified */
#define MAX_MSG_LEN 256

/*
 * Main function
 */
int main(int argc, char *argv[]) {
    int sock_fd;
    struct sockaddr_in server_addr;
    fd_set read_set;
    char buffer[MAX_MSG_LEN];
    int max_fd;
    
    /* Check command line arguments */
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <name>\n", argv[0]);
        exit(1);
    }
    
    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    char *my_name = argv[3];
    
    /* ============================================
     * STEP 1: Create socket and connect to server
     * ============================================ */
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        exit(1);
    }
    
    /* Prepare server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    /* Convert IP address from string to binary */
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(1);
    }
    
    /* Connect to server */
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }
    
    /* ============================================
     * STEP 2: Send our name to the server
     * ============================================ */
    sprintf(buffer, "%s\n", my_name);
    write(sock_fd, buffer, strlen(buffer));
    
    /* ============================================
     * STEP 3: Main loop - handle input and messages
     * ============================================ */
    
    /* Calculate max_fd once (it doesn't change) */
    if (sock_fd > STDIN_FILENO) {
        max_fd = sock_fd;
    } else {
        max_fd = STDIN_FILENO;
    }
    
    while (1) {
        /* Set up the file descriptor set */
        FD_ZERO(&read_set);
        FD_SET(STDIN_FILENO, &read_set);  /* Watch keyboard input */
        FD_SET(sock_fd, &read_set);        /* Watch server socket */
        
        /* Wait for activity */
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            exit(1);
        }
        
        /* ----------------------------------------
         * Check for message from server
         * ---------------------------------------- */
        if (FD_ISSET(sock_fd, &read_set)) {
            memset(buffer, 0, MAX_MSG_LEN);
            int n = read(sock_fd, buffer, MAX_MSG_LEN - 1);
            
            if (n <= 0) {
                /* Server disconnected */
                printf("Server disconnected.\n");
                break;
            }
            
            /* Display the message as-is (spec requirement) */
            printf("%s", buffer);
            fflush(stdout);
        }
        
        /* ----------------------------------------
         * Check for keyboard input from user
         * ---------------------------------------- */
        if (FD_ISSET(STDIN_FILENO, &read_set)) {
            memset(buffer, 0, MAX_MSG_LEN);
            
            if (fgets(buffer, MAX_MSG_LEN, stdin) == NULL) {
                /* EOF or error on stdin */
                break;
            }
            
            /* Check for !exit command */
            if (strncmp(buffer, "!exit", 5) == 0) {
                /* Send to server so others see it */
                write(sock_fd, buffer, strlen(buffer));
                
                /* Print exit message as required by spec */
                printf("client exiting\n");
                break;
            }
            
            /* Send the message to server (normal or whisper) */
            write(sock_fd, buffer, strlen(buffer));
        }
    }
    
    /* Clean up */
    close(sock_fd);
    return 0;
}
