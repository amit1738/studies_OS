/*
 * hw3server.c - Chat Server
 * 
 * This server handles multiple clients using select().
 * - Accepts new connections and registers client names
 * - Broadcasts messages to all clients
 * - Supports private whisper messages (@name message)
 * - Detects and handles client disconnections
 * 
 * Usage: ./hw3server <port>
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
#define MAX_CLIENTS  16
#define MAX_MSG_LEN  256

/* Structure to store information about each connected client */
typedef struct {
    int socket_fd;              /* Socket file descriptor */
    char name[MAX_MSG_LEN];     /* Client's name */
    char ip[INET_ADDRSTRLEN];   /* Client's IP address */
} ClientInfo;

/* Global array of connected clients */
ClientInfo clients[MAX_CLIENTS];
int num_clients = 0;

/*
 * Send a message to a specific client
 */
void send_message(int fd, const char *msg) {
    write(fd, msg, strlen(msg));
}

/*
 * Send a message to ALL connected clients
 */
void broadcast_message(const char *msg) {
    int i;
    for (i = 0; i < num_clients; i++) {
        send_message(clients[i].socket_fd, msg);
    }
}

/*
 * Find a client by name
 * Returns the index in clients array, or -1 if not found
 */
int find_client_by_name(const char *name) {
    int i;
    for (i = 0; i < num_clients; i++) {
        if (strcmp(clients[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * Remove a client from the server
 * Uses the "swap with last" trick to avoid shifting the array
 */
void remove_client(int fd) {
    int i;
    for (i = 0; i < num_clients; i++) {
        if (clients[i].socket_fd == fd) {
            /* Print disconnection message */
            printf("client %s disconnected\n", clients[i].name);
            
            /* Close the socket */
            close(fd);
            
            /* Move the last client to this position (if not already last) */
            if (i < num_clients - 1) {
                clients[i] = clients[num_clients - 1];
            }
            num_clients--;
            break;
        }
    }
}

/*
 * Process a message from a client
 * Handles both normal messages and whisper messages (@name msg)
 */
void process_message(int sender_index, char *message) {
    char formatted_msg[MAX_MSG_LEN * 2];
    char *sender_name = clients[sender_index].name;
    
    /* Remove trailing newline if present */
    int len = strlen(message);
    if (len > 0 && message[len - 1] == '\n') {
        message[len - 1] = '\0';
    }
    
    /* Check if it's a whisper message (starts with @) */
    if (message[0] == '@') {
        /* Parse the target name and message */
        char target_name[MAX_MSG_LEN];
        char *space_ptr;
        
        /* Find the space that separates name from message */
        space_ptr = strchr(message + 1, ' ');
        
        if (space_ptr != NULL) {
            /* Extract target name */
            int name_len = space_ptr - (message + 1);
            strncpy(target_name, message + 1, name_len);
            target_name[name_len] = '\0';
            
            /* Get the actual message (after the space) */
            char *actual_msg = space_ptr + 1;
            
            /* Find the target client */
            int target_index = find_client_by_name(target_name);
            
            if (target_index >= 0) {
                /* Format: "sender: message" and send only to target */
                sprintf(formatted_msg, "%s: %s\n", sender_name, actual_msg);
                send_message(clients[target_index].socket_fd, formatted_msg);
            }
            /* If target not found, message is silently dropped */
        }
    }
    else {
        /* Normal message - broadcast to ALL clients */
        sprintf(formatted_msg, "%s: %s\n", sender_name, message);
        broadcast_message(formatted_msg);
    }
}

/*
 * Main function
 */
int main(int argc, char *argv[]) {
    int listen_fd, new_fd, max_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    fd_set read_set;
    char buffer[MAX_MSG_LEN];
    int i;
    
    /* Check command line arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    
    int port = atoi(argv[1]);
    
    /* ============================================
     * STEP 1: Create the listening socket
     * ============================================ */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(1);
    }
    
    /* Allow port reuse (helpful when restarting server) */
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* ============================================
     * STEP 2: Bind the socket to a port
     * ============================================ */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  /* Accept from any IP */
    server_addr.sin_port = htons(port);
    
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    /* ============================================
     * STEP 3: Start listening for connections
     * ============================================ */
    if (listen(listen_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(1);
    }
    
    /* ============================================
     * STEP 4: Main server loop using select()
     * ============================================ */
    while (1) {
        /* Clear the set and add the listening socket */
        FD_ZERO(&read_set);
        FD_SET(listen_fd, &read_set);
        max_fd = listen_fd;
        
        /* Add all connected client sockets to the set */
        for (i = 0; i < num_clients; i++) {
            FD_SET(clients[i].socket_fd, &read_set);
            if (clients[i].socket_fd > max_fd) {
                max_fd = clients[i].socket_fd;
            }
        }
        
        /* Wait for activity on any socket */
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;  /* Interrupted, just retry */
            perror("select");
            exit(1);
        }
        
        /* ----------------------------------------
         * Check for new incoming connections
         * ---------------------------------------- */
        if (FD_ISSET(listen_fd, &read_set)) {
            client_len = sizeof(client_addr);
            new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
            
            if (new_fd >= 0) {
                /* Read the client's name (sent immediately after connect) */
                memset(buffer, 0, MAX_MSG_LEN);
                int n = read(new_fd, buffer, MAX_MSG_LEN - 1);
                
                if (n > 0 && num_clients < MAX_CLIENTS) {
                    /* Remove newline from name */
                    int len = strlen(buffer);
                    if (len > 0 && buffer[len - 1] == '\n') {
                        buffer[len - 1] = '\0';
                    }
                    
                    /* Store client information */
                    clients[num_clients].socket_fd = new_fd;
                    strcpy(clients[num_clients].name, buffer);
                    inet_ntop(AF_INET, &client_addr.sin_addr, 
                              clients[num_clients].ip, INET_ADDRSTRLEN);
                    
                    /* Print connection message as required by spec */
                    printf("client %s connected from %s\n", 
                           clients[num_clients].name, 
                           clients[num_clients].ip);
                    
                    num_clients++;
                }
                else {
                    /* Either read failed or too many clients */
                    close(new_fd);
                }
            }
        }
        
        /* ----------------------------------------
         * Check for messages from connected clients
         * ---------------------------------------- */
        for (i = 0; i < num_clients; i++) {
            int client_fd = clients[i].socket_fd;
            
            if (FD_ISSET(client_fd, &read_set)) {
                memset(buffer, 0, MAX_MSG_LEN);
                int n = read(client_fd, buffer, MAX_MSG_LEN - 1);
                
                if (n <= 0) {
                    /* Client disconnected or error */
                    remove_client(client_fd);
                    i--;  /* Adjust index since array shifted */
                }
                else {
                    /* Process the received message */
                    process_message(i, buffer);
                }
            }
        }
    }
    
    return 0;
}
