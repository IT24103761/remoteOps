#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9410
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if (argc != 2) {
        printf("Usage: %s <agent_ip>\n", argv[0]);
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        printf("Invalid IP address\n");
        close(sockfd);
        return 1;
    }

    if (connect(
            sockfd,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)
        ) < 0) {

        perror("connect");
        close(sockfd);
        return 1;
    }

    printf("Connected to RemoteOps Agent\n");
    printf("Agent IP: %s\n", argv[1]);
    printf("TCP Port: %d\n", PORT);

    const char *message = "HELLO\n";

    send(
        sockfd,
        message,
        strlen(message),
        0
    );

    memset(buffer, 0, sizeof(buffer));

    int bytes_received = recv(
        sockfd,
        buffer,
        sizeof(buffer) - 1,
        0
    );

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Agent response: %s", buffer);
    }

    close(sockfd);

    return 0;
}
