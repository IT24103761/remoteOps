#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 9410
#define BUFFER_SIZE 8192

typedef struct {
    int socket_fd;
    volatile int running;
    int udp_port;
} UDPListener;


/* =========================================================
   SEND ALL
   ========================================================= */
int send_all(
    int sockfd,
    const void *data,
    size_t length
) {
    const char *ptr =
        (const char *)data;

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


/* =========================================================
   RECEIVE EXACT BYTES
   ========================================================= */
int recv_all(
    int sockfd,
    void *data,
    size_t length
) {
    char *ptr =
        (char *)data;

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


/* =========================================================
   RECEIVE LINE
   ========================================================= */
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


/* =========================================================
   UDP LISTENER THREAD
   ========================================================= */
void *udp_listener_thread(
    void *arg
) {
    UDPListener *listener =
        (UDPListener *)arg;


    char buffer[1024];


    while (listener->running) {
        struct sockaddr_in sender;

        socklen_t sender_len =
            sizeof(sender);


        ssize_t received =
            recvfrom(
                listener->socket_fd,
                buffer,
                sizeof(buffer) - 1,
                0,
                (struct sockaddr *)&sender,
                &sender_len
            );


        if (received > 0) {
            buffer[received] = '\0';

            printf(
                "\n[UDP MONITOR] %s\n",
                buffer
            );

            printf("RemoteOps> ");

            fflush(stdout);
        }
    }


    return NULL;
}


/* =========================================================
   START UDP LISTENER
   ========================================================= */
int start_udp_listener(
    UDPListener *listener,
    pthread_t *thread_id,
    int udp_port
) {
    listener->socket_fd =
        socket(
            AF_INET,
            SOCK_DGRAM,
            0
        );


    if (listener->socket_fd < 0) {
        perror("UDP socket");
        return -1;
    }


    int opt = 1;


    setsockopt(
        listener->socket_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    );


    struct timeval timeout;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;


    setsockopt(
        listener->socket_fd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &timeout,
        sizeof(timeout)
    );


    struct sockaddr_in udp_addr;


    memset(
        &udp_addr,
        0,
        sizeof(udp_addr)
    );


    udp_addr.sin_family =
        AF_INET;

    udp_addr.sin_addr.s_addr =
        INADDR_ANY;

    udp_addr.sin_port =
        htons(udp_port);


    if (bind(
            listener->socket_fd,
            (struct sockaddr *)&udp_addr,
            sizeof(udp_addr)
        ) < 0) {

        perror("UDP bind");

        close(listener->socket_fd);

        return -1;
    }


    listener->running = 1;

    listener->udp_port =
        udp_port;


    if (pthread_create(
            thread_id,
            NULL,
            udp_listener_thread,
            listener
        ) != 0) {

        close(listener->socket_fd);

        return -1;
    }


    printf(
        "UDP listener started on port %d\n",
        udp_port
    );


    return 0;
}


/* =========================================================
   STOP UDP LISTENER
   ========================================================= */
void stop_udp_listener(
    UDPListener *listener,
    pthread_t thread_id
) {
    if (!listener->running) {
        return;
    }


    listener->running = 0;


    pthread_join(
        thread_id,
        NULL
    );


    close(
        listener->socket_fd
    );


    printf(
        "UDP listener stopped.\n"
    );
}


/* =========================================================
   PUT
   ========================================================= */
void upload_file(
    int sockfd,
    const char *filename
) {
    FILE *fp =
        fopen(filename, "rb");


    if (fp == NULL) {
        printf(
            "Local file not found: %s\n",
            filename
        );

        return;
    }


    fseek(
        fp,
        0,
        SEEK_END
    );


    long filesize =
        ftell(fp);


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

        fclose(fp);
        return;
    }


    char file_buffer[4096];

    size_t bytes_read;


    while (
        (bytes_read = fread(
            file_buffer,
            1,
            sizeof(file_buffer),
            fp
        )) > 0
    ) {

        if (send_all(
                sockfd,
                file_buffer,
                bytes_read
            ) < 0) {

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

        printf(
            "Agent disconnected.\n"
        );

        return;
    }


    printf(
        "%s\n",
        response
    );
}


/* =========================================================
   GET
   ========================================================= */
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

        return;
    }


    char response[BUFFER_SIZE];


    if (recv_line(
            sockfd,
            response,
            sizeof(response)
        ) < 0) {

        printf(
            "Agent disconnected.\n"
        );

        return;
    }


    printf(
        "%s\n",
        response
    );


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

        return;
    }


    char output_name[512];


    snprintf(
        output_name,
        sizeof(output_name),
        "downloaded_%s",
        returned_filename
    );


    FILE *fp =
        fopen(
            output_name,
            "wb"
        );


    if (fp == NULL) {
        return;
    }


    long remaining =
        filesize;


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

            return;
        }


        fwrite(
            file_buffer,
            1,
            chunk,
            fp
        );


        remaining -=
            (long)chunk;
    }


    fclose(fp);


    printf(
        "Downloaded as: %s (%ld bytes)\n",
        output_name,
        filesize
    );
}


/* =========================================================
   MAIN
   ========================================================= */
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


    server_addr.sin_family =
        AF_INET;

    server_addr.sin_port =
        htons(PORT);


    if (inet_pton(
            AF_INET,
            argv[1],
            &server_addr.sin_addr
        ) <= 0) {

        printf(
            "Invalid IP address\n"
        );

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


    UDPListener udp_listener;

    memset(
        &udp_listener,
        0,
        sizeof(udp_listener)
    );


    pthread_t udp_thread;

    int udp_active = 0;


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


        /* PUT */
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


        /* GET */
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


        /* MONITOR START */
        if (strncmp(
                input,
                "MONITOR START ",
                14
            ) == 0) {

            int udp_port = 0;


            if (sscanf(
                    input,
                    "MONITOR START %d",
                    &udp_port
                ) != 1 ||
                udp_port < 1 ||
                udp_port > 65535) {

                printf(
                    "Usage: MONITOR START <udp_port>\n"
                );

                continue;
            }


            if (!udp_active) {
                if (start_udp_listener(
                        &udp_listener,
                        &udp_thread,
                        udp_port
                    ) != 0) {

                    printf(
                        "Unable to start UDP listener.\n"
                    );

                    continue;
                }

                udp_active = 1;
            }
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

            printf(
                "Failed to send command.\n"
            );

            break;
        }


        if (recv_line(
                sockfd,
                response,
                sizeof(response)
            ) < 0) {

            printf(
                "Agent disconnected.\n"
            );

            break;
        }


        printf(
            "%s\n",
            response
        );


        /* MONITOR STOP */
        if (strcmp(
                input,
                "MONITOR STOP"
            ) == 0) {

            if (udp_active) {
                stop_udp_listener(
                    &udp_listener,
                    udp_thread
                );

                udp_active = 0;
            }
        }


        if (strcmp(
                input,
                "QUIT"
            ) == 0) {

            break;
        }
    }


    if (udp_active) {
        stop_udp_listener(
            &udp_listener,
            udp_thread
        );
    }


    close(sockfd);

    return 0;
}
