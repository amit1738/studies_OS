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
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(1);
    }
    
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }
    
    sprintf(buffer, "%s\n", my_name);
    write(sock_fd, buffer, strlen(buffer));
    
    if (sock_fd > STDIN_FILENO) {
        max_fd = sock_fd;
    } else {
        max_fd = STDIN_FILENO;
    }
    
    char input_buffer[MAX_MSG_LEN];
    int input_len = 0;

    while (1) {
        FD_ZERO(&read_set);
        FD_SET(STDIN_FILENO, &read_set);
        FD_SET(sock_fd, &read_set);
        
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            exit(1);
        }
        
        if (FD_ISSET(sock_fd, &read_set)) {
            memset(buffer, 0, MAX_MSG_LEN);
            int n = read(sock_fd, buffer, MAX_MSG_LEN - 1);
            
            if (n <= 0) {
                printf("Server disconnected.\n");
                break;
            }
            
            printf("%s", buffer);
            fflush(stdout);
        }
        
        if (FD_ISSET(STDIN_FILENO, &read_set)) {
            int n = read(STDIN_FILENO, input_buffer + input_len, MAX_MSG_LEN - input_len - 1);
            
            if (n <= 0) {
                break;
            }
            
            input_len += n;
            input_buffer[input_len] = '\0';
            
            char *newline;
            while ((newline = strchr(input_buffer, '\n')) != NULL) {
                int msg_len = (newline - input_buffer) + 1;
                
                if (strncmp(input_buffer, "!exit", 5) == 0) {
                    write(sock_fd, input_buffer, msg_len);
                    printf("client exiting\n");
                    close(sock_fd);
                    return 0;
                }
                
                write(sock_fd, input_buffer, msg_len);
                
                int remaining = input_len - msg_len;
                memmove(input_buffer, input_buffer + msg_len, remaining);
                input_len = remaining;
                input_buffer[input_len] = '\0';
            }
            
            if (input_len >= MAX_MSG_LEN - 1) {
                write(sock_fd, input_buffer, input_len);
                input_len = 0;
            }
        }
    }
    
    close(sock_fd);
    return 0;
}
