#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9410
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    char buffer[BUFFER_SIZE];

    // Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // Allow port reuse
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to port 9410
    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {

        perror("bind");
        close(server_fd);
        return 1;
    }

    // Start listening
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("========================================\n");
    printf("RemoteOps Agent\n");
    printf("Registration Number: IT24103761\n");
    printf("Listening on TCP port %d\n", PORT);
    printf("SID:1673\n");
    printf("========================================\n");

    // Accept one Controller for this initial test
    client_fd = accept(
        server_fd,
        (struct sockaddr *)&client_addr,
        &client_len
    );

    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        return 1;
    }

    printf("Controller connected from %s\n",
           inet_ntoa(client_addr.sin_addr));

    memset(buffer, 0, sizeof(buffer));

    int bytes_received = recv(
        client_fd,
        buffer,
        sizeof(buffer) - 1,
        0
    );

    if (bytes_received > 0) {

        buffer[bytes_received] = '\0';

        printf("Received: %s", buffer);

        const char *response =
            "OK REMOTEOPS_CONNECTED SID:1673\n";

        send(
            client_fd,
            response,
            strlen(response),
            0
        );
    }

    close(client_fd);
    close(server_fd);

    return 0;
}
