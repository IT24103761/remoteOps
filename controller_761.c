#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9410
#define BUFFER_SIZE 8192

int send_all(int sockfd, const void *data, size_t length) {
    const char *ptr = (const char *)data;
    size_t total = 0;

    while (total < length) {
        ssize_t sent = send(
            sockfd,
            ptr + total,
            length - total,
            0
        );

        if (sent <= 0) {
            return -1;
        }

        total += (size_t)sent;
    }

    return 0;
}

int recv_all(int sockfd, void *data, size_t length) {
    char *ptr = (char *)data;
    size_t total = 0;

    while (total < length) {
        ssize_t received = recv(
            sockfd,
            ptr + total,
            length - total,
            0
        );

        if (received <= 0) {
            return -1;
        }

        total += (size_t)received;
    }

    return 0;
}

int recv_line(
    int sockfd,
    char *buffer,
    size_t buffer_size
) {
    size_t used = 0;

    while (used < buffer_size - 1) {
        char ch;

        ssize_t result = recv(
            sockfd,
            &ch,
            1,
            0
        );

        if (result <= 0) {
            return -1;
        }

        if (ch == '\n') {
            break;
        }

        if (ch != '\r') {
            buffer[used++] = ch;
        }
    }

    buffer[used] = '\0';

    return (int)used;
}

void upload_file(
    int sockfd,
    const char *filename
) {
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        printf(
            "Local file not found: %s\n",
            filename
        );
        return;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        printf("Unable to determine file size.\n");
        fclose(fp);
        return;
    }

    long filesize = ftell(fp);

    if (filesize < 0) {
        printf("Unable to determine file size.\n");
        fclose(fp);
        return;
    }

    rewind(fp);

    char command[512];

    snprintf(
        command,
        sizeof(command),
        "PUT %s %ld\n",
        filename,
        filesize
    );

    printf(
        "Sending: PUT %s %ld\n",
        filename,
        filesize
    );

    if (send_all(
            sockfd,
            command,
            strlen(command)
        ) < 0) {

        printf("Failed to send PUT command.\n");
        fclose(fp);
        return;
    }

    char file_buffer[4096];
    size_t bytes_read;

    while ((bytes_read = fread(
                file_buffer,
                1,
                sizeof(file_buffer),
                fp
            )) > 0) {

        if (send_all(
                sockfd,
                file_buffer,
                bytes_read
            ) < 0) {

            printf("File upload failed.\n");
            fclose(fp);
            return;
        }
    }

    fclose(fp);

    char response[BUFFER_SIZE];

    if (recv_line(
            sockfd,
            response,
            sizeof(response)
        ) < 0) {

        printf("Agent disconnected.\n");
        return;
    }

    printf("%s\n", response);
}

void download_file(
    int sockfd,
    const char *filename
) {
    char command[512];

    snprintf(
        command,
        sizeof(command),
        "GET %s\n",
        filename
    );

    if (send_all(
            sockfd,
            command,
            strlen(command)
        ) < 0) {

        printf("Failed to send GET request.\n");
        return;
    }

    char response[BUFFER_SIZE];

    if (recv_line(
            sockfd,
            response,
            sizeof(response)
        ) < 0) {

        printf("Agent disconnected.\n");
        return;
    }

    printf("%s\n", response);

    if (strncmp(
            response,
            "OK FILE_SEND ",
            13
        ) != 0) {

        return;
    }

    char returned_filename[256];
    long filesize;

    if (sscanf(
            response,
            "OK FILE_SEND %255s %ld",
            returned_filename,
            &filesize
        ) != 2) {

        printf("Invalid FILE_SEND response.\n");
        return;
    }

    char output_name[512];

    snprintf(
        output_name,
        sizeof(output_name),
        "downloaded_%s",
        returned_filename
    );

    FILE *fp = fopen(output_name, "wb");

    if (fp == NULL) {
        printf("Unable to create downloaded file.\n");
        return;
    }

    long remaining = filesize;
    char file_buffer[4096];

    while (remaining > 0) {
        size_t chunk =
            remaining > (long)sizeof(file_buffer)
            ? sizeof(file_buffer)
            : (size_t)remaining;

        if (recv_all(
                sockfd,
                file_buffer,
                chunk
            ) < 0) {

            fclose(fp);
            printf("Download interrupted.\n");
            return;
        }

        if (fwrite(
                file_buffer,
                1,
                chunk,
                fp
            ) != chunk) {

            fclose(fp);
            printf("Failed to write downloaded file.\n");
            return;
        }

        remaining -= (long)chunk;
    }

    fclose(fp);

    printf(
        "Downloaded as: %s (%ld bytes)\n",
        output_name,
        filesize
    );
}

int main(
    int argc,
    char *argv[]
) {
    int sockfd;
    struct sockaddr_in server_addr;

    if (argc != 2) {
        printf(
            "Usage: %s <agent_ip>\n",
            argv[0]
        );
        return 1;
    }

    sockfd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(
        &server_addr,
        0,
        sizeof(server_addr)
    );

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(
            AF_INET,
            argv[1],
            &server_addr.sin_addr
        ) <= 0) {

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

    printf("========================================\n");
    printf("RemoteOps Controller\n");
    printf("Connected to Agent: %s\n", argv[1]);
    printf("TCP Port: %d\n", PORT);
    printf("========================================\n");

    char input[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    while (1) {
        printf("RemoteOps> ");
        fflush(stdout);

        if (fgets(
                input,
                sizeof(input),
                stdin
            ) == NULL) {

            break;
        }

        input[
            strcspn(input, "\r\n")
        ] = '\0';

        if (input[0] == '\0') {
            continue;
        }

        if (strncmp(
                input,
                "PUT ",
                4
            ) == 0) {

            upload_file(
                sockfd,
                input + 4
            );

            continue;
        }

        if (strncmp(
                input,
                "GET ",
                4
            ) == 0) {

            download_file(
                sockfd,
                input + 4
            );

            continue;
        }

        char protocol_line[
            BUFFER_SIZE + 2
        ];

        snprintf(
            protocol_line,
            sizeof(protocol_line),
            "%s\n",
            input
        );

        if (send_all(
                sockfd,
                protocol_line,
                strlen(protocol_line)
            ) < 0) {

            printf("Failed to send command.\n");
            break;
        }

        if (recv_line(
                sockfd,
                response,
                sizeof(response)
            ) < 0) {

            printf("Agent disconnected.\n");
            break;
        }

        printf("%s\n", response);

        if (strcmp(
                input,
                "QUIT"
            ) == 0) {

            break;
        }
    }

    close(sockfd);

    return 0;
}
