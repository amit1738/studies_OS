// hw3server.c - TCP Chat Server
//
// Server implementation using select() to handle multiple clients
// without threads. All sockets are monitored simultaneously for activity.
//
// Features:
// - Accepts new connections
// - Broadcasts messages to all clients
// - Private whispers (@name msg) are handled
// - Client disconnections are detected

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
#define MAX_CLIENTS  16
#define MAX_MSG_LEN  256

// Structure to maintain client information
typedef struct {
    int socket_fd;              // Client file descriptor
    char name[MAX_MSG_LEN];     // Client chat name
    char ip[INET_ADDRSTRLEN];   // Client IP address
    char buffer[MAX_MSG_LEN];   // Buffer for partial messages (TCP is stream-based)
    int buf_len;                // Current data length in buffer
} ClientInfo;

// Global array to store connected clients
ClientInfo clients[MAX_CLIENTS];
int num_clients = 0;

// Helper function: message is sent to a single client
void send_message(int fd, const char *msg) {
    write(fd, msg, strlen(msg));
}

// Helper function: message is broadcast to all clients
void broadcast_message(const char *msg) {
    int i;
    for (i = 0; i < num_clients; i++) {
        send_message(clients[i].socket_fd, msg);
    }
}

// Linear search: client index is found by name
int find_client_by_name(const char *name) {
    int i;
    for (i = 0; i < num_clients; i++) {
        if (strcmp(clients[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Removes a client upon disconnection.
// Optimization: Instead of shifting the entire array (O(N)),
// the removed client is swapped with the last element.
// Since order is irrelevant, this operation is O(1).
void remove_client(int fd) {
    int i;
    for (i = 0; i < num_clients; i++) {
        if (clients[i].socket_fd == fd) {
            // Print the required disconnection message
            printf("client %s disconnected\n", clients[i].name);
            
            // Close the connection
            close(fd);
            
            // Swap with the last client to fill the gap
            if (i < num_clients - 1) {
                clients[i] = clients[num_clients - 1];
            }
            num_clients--;
            break;
        }
    }
}

// Function to determine if a message is a broadcast or a whisper.
// Messages starting with '@' are parsed as private messages.
void process_message(int sender_index, char *message) {
    char formatted_msg[MAX_MSG_LEN * 2];
    char *sender_name = clients[sender_index].name;
    
    // Remove trailing newline if present
    int len = strlen(message);
    if (len > 0 && message[len - 1] == '\n') {
        message[len - 1] = '\0';
    }
    
    // Whisper syntax (@name message) is checked
    if (message[0] == '@') {
        char target_name[MAX_MSG_LEN];
        char *space_ptr;
        
        // The space separating name and message is located
        space_ptr = strchr(message + 1, ' ');
        
        if (space_ptr != NULL) {
            // The target name is extracted
            int name_len = space_ptr - (message + 1);
            strncpy(target_name, message + 1, name_len);
            target_name[name_len] = '\0';
            
            // The target client is located
            int target_index = find_client_by_name(target_name);
            
            if (target_index >= 0) {
                // Target found. The message is sent exclusively.
                // The specification requires including the original message (with @name).
                sprintf(formatted_msg, "%s: %s\n", sender_name, message);
                send_message(clients[target_index].socket_fd, formatted_msg);
            }
            // If user is not found, the message is silently ignored
        }
    }
    else {
        // Standard message, broadcast to all clients
        sprintf(formatted_msg, "%s: %s\n", sender_name, message);
        broadcast_message(formatted_msg);
    }
}

int main(int argc, char *argv[]) {
    int listen_fd, new_fd, max_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    fd_set read_set;
    char buffer[MAX_MSG_LEN];
    int i;
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    
    int port = atoi(argv[1]);
    
    // The main listening socket is created
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(1);
    }
    
    // Address reuse is enabled to allow immediate server restart
    // without waiting for the OS to release the port.
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // The socket is bound to the port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    server_addr.sin_port = htons(port);
    
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    // Listening for incoming connections is started
    if (listen(listen_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(1);
    }
    
    // Main Server Loop
    // select() is used to monitor activity on the listening socket
    // and all connected client sockets.
    while (1) {
        // The set must be rebuilt each iteration as select() modifies it
        FD_ZERO(&read_set);
        FD_SET(listen_fd, &read_set);
        max_fd = listen_fd;
        
        // Add all current clients to the set
        for (i = 0; i < num_clients; i++) {
            FD_SET(clients[i].socket_fd, &read_set);
            if (clients[i].socket_fd > max_fd) {
                max_fd = clients[i].socket_fd;
            }
        }
        
        // Wait until one of the sockets is ready
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;  // Signal interruption, retry
            perror("select");
            exit(1);
        }
        
        // New connection requests are checked
        if (FD_ISSET(listen_fd, &read_set)) {
            client_len = sizeof(client_addr);
            new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
            
            if (new_fd >= 0) {
                // The client name is sent immediately after connection
                memset(buffer, 0, MAX_MSG_LEN);
                int n = read(new_fd, buffer, MAX_MSG_LEN - 1);
                
                if (n > 0 && num_clients < MAX_CLIENTS) {
                    // The name is sanitized (newline removed)
                    int len = strlen(buffer);
                    if (len > 0 && buffer[len - 1] == '\n') {
                        buffer[len - 1] = '\0';
                    }
                    
                    // Information for the new client is stored
                    clients[num_clients].socket_fd = new_fd;
                    strcpy(clients[num_clients].name, buffer);
                    inet_ntop(AF_INET, &client_addr.sin_addr, 
                              clients[num_clients].ip, INET_ADDRSTRLEN);
                    clients[num_clients].buf_len = 0;  // Initialize empty buffer
                    
                    printf("client %s connected from %s\n", 
                           clients[num_clients].name, 
                           clients[num_clients].ip);
                    
                    num_clients++;
                }
                else {
                    // Error reading name or server capacity reached
                    close(new_fd);
                }
            }
        }
        
        // All clients are checked for incoming messages
        for (i = 0; i < num_clients; i++) {
            int client_fd = clients[i].socket_fd;
            
            if (FD_ISSET(client_fd, &read_set)) {
                // TCP is stream-based, not packet-based.
                // Partial messages or multiple messages may be received.
                // Data is read into a buffer and parsed for newlines.
                int space_left = MAX_MSG_LEN - clients[i].buf_len - 1;
                if (space_left <= 0) {
                    // Buffer full, existing data is processed to prevent stalling
                    clients[i].buffer[clients[i].buf_len] = '\0';
                    process_message(i, clients[i].buffer);
                    clients[i].buf_len = 0;
                    space_left = MAX_MSG_LEN - 1;
                }
                
                int n = read(client_fd, clients[i].buffer + clients[i].buf_len, space_left);
                
                if (n <= 0) {
                    // Client disconnected or error occurred
                    remove_client(client_fd);
                    i--;  // Adjust index as remove_client modified the array
                }
                else {
                    clients[i].buf_len += n;
                    clients[i].buffer[clients[i].buf_len] = '\0';
                    
                    // All complete messages in the buffer are processed
                    char *newline;
                    while ((newline = strchr(clients[i].buffer, '\n')) != NULL) {
                        // Message length is calculated
                        int msg_len = newline - clients[i].buffer + 1;
                        char message[MAX_MSG_LEN];
                        strncpy(message, clients[i].buffer, msg_len);
                        message[msg_len] = '\0';
                        
                        // The message is handled
                        process_message(i, message);
                        
                        // Remaining data is moved to the start of the buffer
                        int remaining = clients[i].buf_len - msg_len;
                        memmove(clients[i].buffer, clients[i].buffer + msg_len, remaining);
                        clients[i].buf_len = remaining;
                        clients[i].buffer[clients[i].buf_len] = '\0';
                    }
                }
            }
        }
    }
    
    return 0;
}
