#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define PORT 9410
#define BUFFER_SIZE 8192
#define AUTH_TOKEN "OPS-3761"
#define SID_TAG "SID:1673"

#define STORAGE_DIR "./agentfiles/IT24103761"
#define MAX_FILE_SIZE (10 * 1024 * 1024)

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} ClientInfo;

int send_all(int sockfd, const void *data, size_t length) {
    const char *ptr = (const char *)data;
    size_t total = 0;

    while (total < length) {
        ssize_t sent = send(sockfd, ptr + total, length - total, 0);

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
        ssize_t received = recv(sockfd, ptr + total, length - total, 0);

        if (received <= 0) {
            return -1;
        }

        total += (size_t)received;
    }

    return 0;
}

int recv_line(int sockfd, char *buffer, size_t buffer_size) {
    size_t used = 0;

    while (used < buffer_size - 1) {
        char ch;

        ssize_t result = recv(sockfd, &ch, 1, 0);

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

int valid_filename(const char *filename) {
    if (filename == NULL || filename[0] == '\0') {
        return 0;
    }

    if (strstr(filename, "..") != NULL) {
        return 0;
    }

    if (strchr(filename, '/') != NULL) {
        return 0;
    }

    return 1;
}

void get_sysinfo(char *response, size_t response_size) {
    double cpu_load = 0.0;
    double uptime = 0.0;

    unsigned long mem_total = 0;
    unsigned long mem_available = 0;
    unsigned long mem_used_mb = 0;

    FILE *fp;

    fp = fopen("/proc/loadavg", "r");
    if (fp != NULL) {
        fscanf(fp, "%lf", &cpu_load);
        fclose(fp);
    }

    fp = fopen("/proc/uptime", "r");
    if (fp != NULL) {
        fscanf(fp, "%lf", &uptime);
        fclose(fp);
    }

    fp = fopen("/proc/meminfo", "r");
    if (fp != NULL) {
        char line[256];

        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "MemTotal: %lu kB", &mem_total) == 1) {
                continue;
            }

            if (sscanf(line, "MemAvailable: %lu kB", &mem_available) == 1) {
                continue;
            }
        }

        fclose(fp);
    }

    if (mem_total >= mem_available) {
        mem_used_mb = (mem_total - mem_available) / 1024;
    }

    snprintf(
        response,
        response_size,
        "OK SYSINFO %.2f %lu %.0f SID:1673\n",
        cpu_load,
        mem_used_mb,
        uptime
    );
}

void get_process_list(char *response, size_t response_size) {
    FILE *fp;
    char line[256];
    char process_data[7000];
    size_t used = 0;

    memset(process_data, 0, sizeof(process_data));

    fp = popen("ps -eo pid,comm --no-headers", "r");

    if (fp == NULL) {
        snprintf(
            response,
            response_size,
            "ERR 007 PROCESS_LIST_FAILED SID:1673\n"
        );
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';

        size_t len = strlen(line);

        if (used + len + 2 >= sizeof(process_data)) {
            break;
        }

        if (used > 0) {
            process_data[used++] = ',';
        }

        memcpy(process_data + used, line, len);
        used += len;
        process_data[used] = '\0';
    }

    pclose(fp);

    snprintf(
        response,
        response_size,
        "OK PROCS %s SID:1673\n",
        process_data
    );
}

void execute_allowed_command(
    const char *name,
    char *response,
    size_t response_size
) {
    const char *command = NULL;

    if (strcmp(name, "DATE") == 0) {
        command = "date";
    } else if (strcmp(name, "UPTIME") == 0) {
        command = "uptime";
    } else if (strcmp(name, "DISKFREE") == 0) {
        command = "df -h";
    } else if (strcmp(name, "HOSTNAME") == 0) {
        command = "hostname";
    } else if (strcmp(name, "WHOAMI") == 0) {
        command = "whoami";
    } else {
        snprintf(
            response,
            response_size,
            "ERR 002 COMMAND_NOT_ALLOWED SID:1673\n"
        );
        return;
    }

    FILE *fp = popen(command, "r");

    if (fp == NULL) {
        snprintf(
            response,
            response_size,
            "ERR 008 EXEC_FAILED SID:1673\n"
        );
        return;
    }

    char output[6000];
    char line[512];
    size_t used = 0;

    memset(output, 0, sizeof(output));

    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\r\n")] = ' ';

        size_t len = strlen(line);

        if (used + len + 1 >= sizeof(output)) {
            break;
        }

        memcpy(output + used, line, len);
        used += len;
        output[used] = '\0';
    }

    pclose(fp);

    snprintf(
        response,
        response_size,
        "OK EXEC_RESULT %s SID:1673\n",
        output
    );
}

void handle_put(int client_fd, const char *filename, long filesize) {
    char response[BUFFER_SIZE];

    if (!valid_filename(filename)) {
        snprintf(
            response,
            sizeof(response),
            "ERR 009 INVALID_FILENAME SID:1673\n"
        );
        send_all(client_fd, response, strlen(response));
        return;
    }

    if (filesize < 0 || filesize > MAX_FILE_SIZE) {
        snprintf(
            response,
            sizeof(response),
            "ERR 004 FILE_TOO_LARGE SID:1673\n"
        );
        send_all(client_fd, response, strlen(response));
        return;
    }

    char path[1024];

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        STORAGE_DIR,
        filename
    );

    FILE *fp = fopen(path, "wb");

    if (fp == NULL) {
        snprintf(
            response,
            sizeof(response),
            "ERR 010 FILE_WRITE_FAILED SID:1673\n"
        );
        send_all(client_fd, response, strlen(response));
        return;
    }

    long remaining = filesize;
    char file_buffer[4096];

    while (remaining > 0) {
        size_t chunk =
            remaining > (long)sizeof(file_buffer)
            ? sizeof(file_buffer)
            : (size_t)remaining;

        if (recv_all(client_fd, file_buffer, chunk) < 0) {
            fclose(fp);
            printf("File upload interrupted: %s\n", filename);
            return;
        }

        if (fwrite(file_buffer, 1, chunk, fp) != chunk) {
            fclose(fp);
            return;
        }

        remaining -= (long)chunk;
    }

    fclose(fp);

    snprintf(
        response,
        sizeof(response),
        "OK FILE_RECEIVED %s SID:1673\n",
        filename
    );

    send_all(client_fd, response, strlen(response));

    printf(
        "Uploaded file: %s (%ld bytes)\n",
        filename,
        filesize
    );
}

void handle_get(int client_fd, const char *filename) {
    char response[BUFFER_SIZE];

    if (!valid_filename(filename)) {
        snprintf(
            response,
            sizeof(response),
            "ERR 009 INVALID_FILENAME SID:1673\n"
        );
        send_all(client_fd, response, strlen(response));
        return;
    }

    char path[1024];

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        STORAGE_DIR,
        filename
    );

    FILE *fp = fopen(path, "rb");

    if (fp == NULL) {
        snprintf(
            response,
            sizeof(response),
            "ERR 005 FILE_NOT_FOUND SID:1673\n"
        );
        send_all(client_fd, response, strlen(response));
        return;
    }

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    rewind(fp);

    snprintf(
        response,
        sizeof(response),
        "OK FILE_SEND %s %ld SID:1673\n",
        filename,
        filesize
    );

    if (send_all(client_fd, response, strlen(response)) < 0) {
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
                client_fd,
                file_buffer,
                bytes_read
            ) < 0) {

            fclose(fp);
            return;
        }
    }

    fclose(fp);

    printf(
        "Downloaded file: %s (%ld bytes)\n",
        filename,
        filesize
    );
}

void *handle_client(void *arg) {
    ClientInfo *client_info = (ClientInfo *)arg;

    int client_fd = client_info->client_fd;

    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(
        AF_INET,
        &client_info->client_addr.sin_addr,
        client_ip,
        sizeof(client_ip)
    );

    free(client_info);

    printf(
        "Controller connected from %s [Thread %lu]\n",
        client_ip,
        (unsigned long)pthread_self()
    );

    int authenticated = 0;
    char buffer[BUFFER_SIZE];

    while (1) {
        int line_length = recv_line(
            client_fd,
            buffer,
            sizeof(buffer)
        );

        if (line_length < 0) {
            printf(
                "Controller %s disconnected [Thread %lu]\n",
                client_ip,
                (unsigned long)pthread_self()
            );
            break;
        }

        printf(
            "[Thread %lu] Received: %s\n",
            (unsigned long)pthread_self(),
            buffer
        );

        if (!authenticated) {
            if (strncmp(buffer, "AUTH ", 5) == 0) {
                char *token = buffer + 5;

                if (strcmp(token, AUTH_TOKEN) == 0) {
                    authenticated = 1;

                    const char *response =
                        "OK AUTHENTICATED SID:1673\n";

                    send_all(
                        client_fd,
                        response,
                        strlen(response)
                    );
                } else {
                    const char *response =
                        "ERR 001 AUTH_FAILED SID:1673\n";

                    send_all(
                        client_fd,
                        response,
                        strlen(response)
                    );
                }
            } else {
                const char *response =
                    "ERR 003 NOT_AUTHENTICATED SID:1673\n";

                send_all(
                    client_fd,
                    response,
                    strlen(response)
                );
            }

            continue;
        }

        if (strcmp(buffer, "SYSINFO") == 0) {
            char response[BUFFER_SIZE];

            get_sysinfo(
                response,
                sizeof(response)
            );

            send_all(
                client_fd,
                response,
                strlen(response)
            );

            continue;
        }

        if (strcmp(buffer, "LISTPROC") == 0) {
            char response[BUFFER_SIZE];

            get_process_list(
                response,
                sizeof(response)
            );

            send_all(
                client_fd,
                response,
                strlen(response)
            );

            continue;
        }

        if (strncmp(buffer, "EXEC ", 5) == 0) {
            char response[BUFFER_SIZE];

            execute_allowed_command(
                buffer + 5,
                response,
                sizeof(response)
            );

            send_all(
                client_fd,
                response,
                strlen(response)
            );

            continue;
        }

        if (strncmp(buffer, "PUT ", 4) == 0) {
            char filename[256];
            long filesize;

            if (sscanf(
                    buffer,
                    "PUT %255s %ld",
                    filename,
                    &filesize
                ) == 2) {

                handle_put(
                    client_fd,
                    filename,
                    filesize
                );
            } else {
                const char *response =
                    "ERR 011 BAD_PUT_FORMAT SID:1673\n";

                send_all(
                    client_fd,
                    response,
                    strlen(response)
                );
            }

            continue;
        }

        if (strncmp(buffer, "GET ", 4) == 0) {
            char filename[256];

            if (sscanf(
                    buffer,
                    "GET %255s",
                    filename
                ) == 1) {

                handle_get(
                    client_fd,
                    filename
                );
            } else {
                const char *response =
                    "ERR 012 BAD_GET_FORMAT SID:1673\n";

                send_all(
                    client_fd,
                    response,
                    strlen(response)
                );
            }

            continue;
        }

        if (strcmp(buffer, "QUIT") == 0) {
            const char *response =
                "OK BYE SID:1673\n";

            send_all(
                client_fd,
                response,
                strlen(response)
            );

            break;
        }

        const char *response =
            "ERR 006 UNKNOWN_COMMAND SID:1673\n";

        send_all(
            client_fd,
            response,
            strlen(response)
        );
    }

    close(client_fd);

    return NULL;
}

int main(void) {
    int server_fd;
    struct sockaddr_in server_addr;

    mkdir("./agentfiles", 0755);
    mkdir(STORAGE_DIR, 0755);

    server_fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;

    if (setsockopt(
            server_fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt)
        ) < 0) {

        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    memset(
        &server_addr,
        0,
        sizeof(server_addr)
    );

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(
            server_fd,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)
        ) < 0) {

        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(
            server_fd,
            10
        ) < 0) {

        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("========================================\n");
    printf("RemoteOps Agent\n");
    printf("Registration Number: IT24103761\n");
    printf("Listening on TCP port %d\n", PORT);
    printf("%s\n", SID_TAG);
    printf("Concurrency Model: POSIX Threads\n");
    printf("Storage: %s\n", STORAGE_DIR);
    printf("========================================\n");

    while (1) {
        ClientInfo *client_info =
            malloc(sizeof(ClientInfo));

        if (client_info == NULL) {
            perror("malloc");
            continue;
        }

        socklen_t client_len =
            sizeof(client_info->client_addr);

        client_info->client_fd = accept(
            server_fd,
            (struct sockaddr *)&client_info->client_addr,
            &client_len
        );

        if (client_info->client_fd < 0) {
            perror("accept");
            free(client_info);
            continue;
        }

        pthread_t thread_id;

        if (pthread_create(
                &thread_id,
                NULL,
                handle_client,
                client_info
            ) != 0) {

            fprintf(
                stderr,
                "pthread_create failed\n"
            );

            close(client_info->client_fd);
            free(client_info);
            continue;
        }

        pthread_detach(thread_id);
    }

    close(server_fd);

    return 0;
}
