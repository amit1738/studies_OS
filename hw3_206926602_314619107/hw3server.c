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

#define MAX_CLIENTS  16
#define MAX_MSG_LEN  256

typedef struct {
    int socket_fd;
    char name[MAX_MSG_LEN];
    char ip[INET_ADDRSTRLEN];
    char buffer[MAX_MSG_LEN];
    int buf_len;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
int num_clients = 0;

void send_message(int fd, const char *msg) {
    write(fd, msg, strlen(msg));
}

void broadcast_message(const char *msg) {
    int i;
    for (i = 0; i < num_clients; i++) {
        send_message(clients[i].socket_fd, msg);
    }
}

int find_client_by_name(const char *name) {
    int i;
    for (i = 0; i < num_clients; i++) {
        if (strcmp(clients[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void remove_client(int fd) {
    int i;
    for (i = 0; i < num_clients; i++) {
        if (clients[i].socket_fd == fd) {
            printf("client %s disconnected\n", clients[i].name);
            close(fd);
            if (i < num_clients - 1) {
                clients[i] = clients[num_clients - 1];
            }
            num_clients--;
            break;
        }
    }
}

void process_message(int sender_index, char *message) {
    char formatted_msg[MAX_MSG_LEN * 2];
    char *sender_name = clients[sender_index].name;
    
    int len = strlen(message);
    if (len > 0 && message[len - 1] == '\n') {
        message[len - 1] = '\0';
    }
    
    if (message[0] == '@') {
        char target_name[MAX_MSG_LEN];
        char *space_ptr;
        
        space_ptr = strchr(message + 1, ' ');
        
        if (space_ptr != NULL) {
            int name_len = space_ptr - (message + 1);
            strncpy(target_name, message + 1, name_len);
            target_name[name_len] = '\0';
            
            int target_index = find_client_by_name(target_name);
            
            if (target_index >= 0) {
                sprintf(formatted_msg, "%s: %s\n", sender_name, message);
                send_message(clients[target_index].socket_fd, formatted_msg);
            }
        }
    }
    else {
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
    
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(1);
    }
    
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    if (listen(listen_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(1);
    }
    
    while (1) {
        FD_ZERO(&read_set);
        FD_SET(listen_fd, &read_set);
        max_fd = listen_fd;
        
        for (i = 0; i < num_clients; i++) {
            FD_SET(clients[i].socket_fd, &read_set);
            if (clients[i].socket_fd > max_fd) {
                max_fd = clients[i].socket_fd;
            }
        }
        
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            exit(1);
        }
        
        if (FD_ISSET(listen_fd, &read_set)) {
            client_len = sizeof(client_addr);
            new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
            
            if (new_fd >= 0) {
                memset(buffer, 0, MAX_MSG_LEN);
                int n = read(new_fd, buffer, MAX_MSG_LEN - 1);
                
                if (n > 0 && num_clients < MAX_CLIENTS) {
                    int len = strlen(buffer);
                    if (len > 0 && buffer[len - 1] == '\n') {
                        buffer[len - 1] = '\0';
                    }
                    
                    clients[num_clients].socket_fd = new_fd;
                    strcpy(clients[num_clients].name, buffer);
                    inet_ntop(AF_INET, &client_addr.sin_addr, 
                              clients[num_clients].ip, INET_ADDRSTRLEN);
                    clients[num_clients].buf_len = 0;
                    
                    printf("client %s connected from %s\n", 
                           clients[num_clients].name, 
                           clients[num_clients].ip);
                    
                    num_clients++;
                }
                else {
                    close(new_fd);
                }
            }
        }
        
        for (i = 0; i < num_clients; i++) {
            int client_fd = clients[i].socket_fd;
            
            if (FD_ISSET(client_fd, &read_set)) {
                int space_left = MAX_MSG_LEN - clients[i].buf_len - 1;
                if (space_left <= 0) {
                    clients[i].buffer[clients[i].buf_len] = '\0';
                    process_message(i, clients[i].buffer);
                    clients[i].buf_len = 0;
                    space_left = MAX_MSG_LEN - 1;
                }
                
                int n = read(client_fd, clients[i].buffer + clients[i].buf_len, space_left);
                
                if (n <= 0) {
                    remove_client(client_fd);
                    i--;
                }
                else {
                    clients[i].buf_len += n;
                    clients[i].buffer[clients[i].buf_len] = '\0';
                    
                    char *newline;
                    while ((newline = strchr(clients[i].buffer, '\n')) != NULL) {
                        int msg_len = newline - clients[i].buffer + 1;
                        char message[MAX_MSG_LEN];
                        strncpy(message, clients[i].buffer, msg_len);
                        message[msg_len] = '\0';
                        
                        process_message(i, message);
                        
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
