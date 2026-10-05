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

    char send_buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];

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

    if (inet_pton(AF_INET,
                  argv[1],
                  &server_addr.sin_addr) <= 0) {

        printf("Invalid IP address\n");
        close(sockfd);
        return 1;
    }

    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {

        perror("connect");
        close(sockfd);
        return 1;
    }

    printf("========================================\n");
    printf("RemoteOps Controller\n");
    printf("Connected to Agent: %s\n", argv[1]);
    printf("TCP Port: %d\n", PORT);
    printf("========================================\n");

    while (1) {

        printf("RemoteOps> ");
        fflush(stdout);

        if (fgets(send_buffer,
                  sizeof(send_buffer),
                  stdin) == NULL) {
            break;
        }

        if (send(sockfd,
                 send_buffer,
                 strlen(send_buffer),
                 0) < 0) {

            perror("send");
            break;
        }

        memset(recv_buffer, 0, sizeof(recv_buffer));

        int bytes_received = recv(
            sockfd,
            recv_buffer,
            sizeof(recv_buffer) - 1,
            0
        );

        if (bytes_received <= 0) {
            printf("Agent disconnected.\n");
            break;
        }

        recv_buffer[bytes_received] = '\0';

        printf("%s", recv_buffer);

        if (strncmp(send_buffer, "QUIT", 4) == 0) {
            break;
        }
    }

    close(sockfd);

    return 0;
}
