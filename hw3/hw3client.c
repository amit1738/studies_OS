#include "common.h"

int main(int argc, char *argv[]) {
    // Syntax: hw3client addr port name [cite: 11]
    if (argc < 4) {
        fprintf(stderr, "Usage: %s addr port name\n", argv[0]);
        exit(1);
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    char *client_name = argv[3];

    int sock_fd;
    struct sockaddr_in serv_addr;
    char buffer[MAX_BUFFER];
    fd_set readfds;

    // 1. Create Socket and Connect [cite: 27]
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) 
        error("Invalid address/ Address not supported");

    if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
        error("Connection Failed");

    // 2. Notify name to server [cite: 27]
    // Sending name with newline to ensure server reads it correctly
    snprintf(buffer, sizeof(buffer), "%s\n", client_name);
    write(sock_fd, buffer, strlen(buffer));

    // 3. Main Loop: Monitor Socket and Stdin [cite: 34]
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // Watch stdin (fd 0)
        FD_SET(sock_fd, &readfds);      // Watch socket

        int max_fd = (sock_fd > STDIN_FILENO) ? sock_fd : STDIN_FILENO;

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            error("ERROR on select");
        }

        // Handle Incoming Message from Server [cite: 28]
        if (FD_ISSET(sock_fd, &readfds)) {
            memset(buffer, 0, MAX_BUFFER);
            int n = read(sock_fd, buffer, MAX_BUFFER - 1);
            if (n <= 0) {
                printf("Server disconnected.\n");
                break;
            }
            printf("%s", buffer); // Display incoming message as is [cite: 28]
            // Ensure output is flushed if newline is missing
            if (buffer[n-1] != '\n') printf("\n");
        }

        // Handle User Input [cite: 29]
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, MAX_BUFFER);
            if (fgets(buffer, MAX_BUFFER, stdin) != NULL) {
                
                // Check for !exit [cite: 33]
                if (strncmp(buffer, "!exit", 5) == 0 && (buffer[5] == '\n' || buffer[5] == '\0')) {
                    write(sock_fd, buffer, strlen(buffer)); // Send to server
                    printf("client exiting\n"); // Print exit message [cite: 33]
                    close(sock_fd);
                    exit(0);
                }

                // Send normal or whisper message [cite: 31, 32]
                write(sock_fd, buffer, strlen(buffer));
            }
        }
    }

    close(sock_fd);
    return 0;
}