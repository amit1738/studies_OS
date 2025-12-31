// hw3client.c - TCP Chat Client
//
// Client implementation for the chat server.
// Simultaneous handling of two input sources is required:
// 1. User input (stdin)
// 2. Incoming server messages (socket)
// select() is used to monitor both file descriptors to prevent blocking.

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

// Constants from the spec
#define MAX_MSG_LEN 256

int main(int argc, char *argv[]) {
    int sock_fd;
    struct sockaddr_in server_addr;
    fd_set read_set;
    char buffer[MAX_MSG_LEN];
    int max_fd;
    
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <name>\n", argv[0]);
        exit(1);
    }
    
    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    char *my_name = argv[3];
    
    // The socket is created
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        exit(1);
    }
    
    // The server address structure is set up
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // The IP string is converted to binary
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(1);
    }
    
    // Connection to the server is established
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }
    
    // The client name is sent immediately upon connection
    sprintf(buffer, "%s\n", my_name);
    write(sock_fd, buffer, strlen(buffer));
    
    // The max file descriptor for select() is determined
    // sock_fd and STDIN_FILENO (0) are monitored
    if (sock_fd > STDIN_FILENO) {
        max_fd = sock_fd;
    } else {
        max_fd = STDIN_FILENO;
    }
    
    // Buffer for user input (rapid typing is handled)
    char input_buffer[MAX_MSG_LEN];
    int input_len = 0;

    // Main Loop
    // Activity on either stdin or the socket is awaited.
    while (1) {
        FD_ZERO(&read_set);
        FD_SET(STDIN_FILENO, &read_set);  // Keyboard is monitored
        FD_SET(sock_fd, &read_set);       // Server is monitored
        
        // Activity is awaited
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            exit(1);
        }
        
        // Case 1: Message is received from server
        if (FD_ISSET(sock_fd, &read_set)) {
            memset(buffer, 0, MAX_MSG_LEN);
            int n = read(sock_fd, buffer, MAX_MSG_LEN - 1);
            
            if (n <= 0) {
                // Connection closed by server
                printf("Server disconnected.\n");
                break;
            }
            
            // The received message is printed
            printf("%s", buffer);
            fflush(stdout);
        }
        
        // Case 2: User input is detected
        if (FD_ISSET(STDIN_FILENO, &read_set)) {
            // Data is read from keyboard into buffer
            int n = read(STDIN_FILENO, input_buffer + input_len, MAX_MSG_LEN - input_len - 1);
            
            if (n <= 0) {
                break; // EOF (Ctrl+D) or error
            }
            
            input_len += n;
            input_buffer[input_len] = '\0';
            
            // Complete lines ending in newline are processed
            char *newline;
            while ((newline = strchr(input_buffer, '\n')) != NULL) {
                int msg_len = (newline - input_buffer) + 1;
                
                // The special exit command is checked
                if (strncmp(input_buffer, "!exit", 5) == 0) {
                    // Server is notified of departure
                    write(sock_fd, input_buffer, msg_len);
                    printf("client exiting\n");
                    close(sock_fd);
                    return 0;
                }
                
                // The message is sent
                write(sock_fd, input_buffer, msg_len);
                
                // Buffer is shifted if data remains
                int remaining = input_len - msg_len;
                memmove(input_buffer, input_buffer + msg_len, remaining);
                input_len = remaining;
                input_buffer[input_len] = '\0';
            }
            
            // Safety: if buffer fills without a newline, it is sent immediately
            if (input_len >= MAX_MSG_LEN - 1) {
                write(sock_fd, input_buffer, input_len);
                input_len = 0;
            }
        }
    }
    
    close(sock_fd);
    return 0;
}
