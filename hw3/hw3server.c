#include "common.h"

// Structure to track connected clients
typedef struct {
    int fd;
    char name[MAX_NAME_LEN];
    char ip[INET_ADDRSTRLEN];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;

// Remove client from list and close socket
void remove_client(int fd) {
    int i;
    for (i = 0; i < client_count; i++) {
        if (clients[i].fd == fd) {
            printf("client %s disconnected\n", clients[i].name); // [cite: 25]
            close(clients[i].fd);
            // Move last client to this spot to keep array packed
            clients[i] = clients[client_count - 1];
            client_count--;
            break;
        }
    }
}

// Helper to send data
void send_to_fd(int fd, char *msg) {
    if (write(fd, msg, strlen(msg)) < 0) {
        perror("write");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) { // [cite: 7]
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int listen_fd, new_fd, max_fd, i, j;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    fd_set readfds;
    char buffer[MAX_BUFFER];
    char msg_buffer[MAX_BUFFER + MAX_NAME_LEN];

    // 1. Setup Server Socket [cite: 16]
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) error("ERROR opening socket");

    // Allow reuse of port to avoid "Address already in use" errors
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(listen_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(listen_fd, 5);

    // Main Server Loop
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listen_fd, &readfds);
        max_fd = listen_fd;

        // Add child sockets to set [cite: 18]
        for (i = 0; i < client_count; i++) {
            int sd = clients[i].fd;
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_fd) max_fd = sd;
        }

        // Wait for activity
        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            error("ERROR on select");
        }

        // 2. Handle New Connections [cite: 16]
        if (FD_ISSET(listen_fd, &readfds)) {
            clilen = sizeof(cli_addr);
            new_fd = accept(listen_fd, (struct sockaddr *) &cli_addr, &clilen);
            
            if (new_fd >= 0) {
                // Protocol: Client sends name immediately after connect
                memset(buffer, 0, MAX_BUFFER);
                if (read(new_fd, buffer, MAX_NAME_LEN) > 0) {
                    // Remove newline if present
                    buffer[strcspn(buffer, "\n")] = 0;
                    
                    if (client_count < MAX_CLIENTS) {
                        clients[client_count].fd = new_fd;
                        strncpy(clients[client_count].name, buffer, MAX_NAME_LEN);
                        inet_ntop(AF_INET, &(cli_addr.sin_addr), clients[client_count].ip, INET_ADDRSTRLEN);
                        
                        // Output required log message [cite: 17]
                        printf("client %s connected from %s\n", clients[client_count].name, clients[client_count].ip);
                        client_count++;
                    } else {
                        close(new_fd); // Too many clients
                    }
                } else {
                    close(new_fd);
                }
            }
        }

        // 3. Handle IO from clients
        for (i = 0; i < client_count; i++) {
            int sd = clients[i].fd;
            if (FD_ISSET(sd, &readfds)) {
                memset(buffer, 0, MAX_BUFFER);
                int n = read(sd, buffer, MAX_BUFFER - 1); // Leave space for null

                if (n <= 0) {
                    // Disconnect [cite: 25]
                    remove_client(sd);
                    i--; // Adjust index since list shifted
                } else {
                    // Process Message
                    // Format: "sourcename: message" [cite: 23]
                    
                    // Check for Whisper [cite: 21]
                    if (buffer[0] == '@') {
                        char *target_name = buffer + 1; // Skip '@'
                        char *msg_ptr = strchr(target_name, ' ');
                        
                        if (msg_ptr != NULL) {
                            *msg_ptr = 0; // Terminate name string temporarily
                            msg_ptr++; // Move past space to message
                            
                            int found = 0;
                            for (j = 0; j < client_count; j++) {
                                if (strcmp(clients[j].name, target_name) == 0) {
                                    snprintf(msg_buffer, sizeof(msg_buffer), "%s: %s", clients[i].name, msg_ptr);
                                    send_to_fd(clients[j].fd, msg_buffer); // Send to target [cite: 20]
                                    found = 1;
                                    break;
                                }
                            }
                        } else {
                            // Malformed whisper, treat as normal or ignore (not specified, treating as normal below)
                            snprintf(msg_buffer, sizeof(msg_buffer), "%s: %s", clients[i].name, buffer);
                            for (j = 0; j < client_count; j++) send_to_fd(clients[j].fd, msg_buffer);
                        }
                    } 
                    // Normal Message [cite: 19]
                    else {
                        snprintf(msg_buffer, sizeof(msg_buffer), "%s: %s", clients[i].name, buffer);
                        // Send to ALL clients
                        for (j = 0; j < client_count; j++) {
                            send_to_fd(clients[j].fd, msg_buffer);
                        }
                    }
                }
            }
        }
    }
    return 0;
}