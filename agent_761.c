#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9410
#define BUFFER_SIZE 8192
#define AUTH_TOKEN "OPS-3761"
#define SID_TAG "SID:1673"

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} ClientInfo;

/* ---------------------------------------------------------
   Read system information from Linux /proc files.
   --------------------------------------------------------- */
void get_sysinfo(char *response, size_t response_size) {
    double cpu_load = 0.0;
    double uptime = 0.0;

    unsigned long mem_total = 0;
    unsigned long mem_available = 0;
    unsigned long mem_used_mb = 0;

    FILE *fp;

    /* CPU load */
    fp = fopen("/proc/loadavg", "r");

    if (fp != NULL) {
        fscanf(fp, "%lf", &cpu_load);
        fclose(fp);
    }

    /* Uptime */
    fp = fopen("/proc/uptime", "r");

    if (fp != NULL) {
        fscanf(fp, "%lf", &uptime);
        fclose(fp);
    }

    /* Memory */
    fp = fopen("/proc/meminfo", "r");

    if (fp != NULL) {
        char line[256];

        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "MemTotal: %lu kB", &mem_total) == 1) {
                continue;
            }

            if (sscanf(line, "MemAvailable: %lu kB",
                       &mem_available) == 1) {
                continue;
            }
        }

        fclose(fp);
    }

    if (mem_total >= mem_available) {
        mem_used_mb =
            (mem_total - mem_available) / 1024;
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

/* ---------------------------------------------------------
   Generate a process snapshot using ps.
   --------------------------------------------------------- */
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

        /* Remove newline */
        line[strcspn(line, "\r\n")] = '\0';

        size_t line_length = strlen(line);

        if (used + line_length + 2 >= sizeof(process_data)) {
            break;
        }

        if (used > 0) {
            process_data[used++] = ',';
        }

        memcpy(
            process_data + used,
            line,
            line_length
        );

        used += line_length;

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

/* ---------------------------------------------------------
   Handle one Controller connection.
   Every Controller receives its own thread.
   --------------------------------------------------------- */
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

        memset(buffer, 0, sizeof(buffer));

        int bytes_received = recv(
            client_fd,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        if (bytes_received <= 0) {

            printf(
                "Controller %s disconnected [Thread %lu]\n",
                client_ip,
                (unsigned long)pthread_self()
            );

            break;
        }

        buffer[bytes_received] = '\0';

        /* Current implementation processes one line command.
           Later stages can extend framing handling if required. */
        buffer[strcspn(buffer, "\r\n")] = '\0';

        printf(
            "[Thread %lu] Received: %s\n",
            (unsigned long)pthread_self(),
            buffer
        );

        /* Authentication ---------------------------------- */

        if (!authenticated) {

            if (strncmp(buffer, "AUTH ", 5) == 0) {

                char *token = buffer + 5;

                if (strcmp(token, AUTH_TOKEN) == 0) {

                    authenticated = 1;

                    const char *response =
                        "OK AUTHENTICATED SID:1673\n";

                    send(
                        client_fd,
                        response,
                        strlen(response),
                        0
                    );

                } else {

                    const char *response =
                        "ERR 001 AUTH_FAILED SID:1673\n";

                    send(
                        client_fd,
                        response,
                        strlen(response),
                        0
                    );
                }

            } else {

                const char *response =
                    "ERR 003 NOT_AUTHENTICATED SID:1673\n";

                send(
                    client_fd,
                    response,
                    strlen(response),
                    0
                );
            }

            continue;
        }

        /* SYSINFO ----------------------------------------- */

        if (strcmp(buffer, "SYSINFO") == 0) {

            char response[BUFFER_SIZE];

            get_sysinfo(
                response,
                sizeof(response)
            );

            send(
                client_fd,
                response,
                strlen(response),
                0
            );

            continue;
        }

        /* LISTPROC ---------------------------------------- */

        if (strcmp(buffer, "LISTPROC") == 0) {

            char response[BUFFER_SIZE];

            get_process_list(
                response,
                sizeof(response)
            );

            send(
                client_fd,
                response,
                strlen(response),
                0
            );

            continue;
        }

        /* QUIT -------------------------------------------- */

        if (strcmp(buffer, "QUIT") == 0) {

            const char *response =
                "OK BYE SID:1673\n";

            send(
                client_fd,
                response,
                strlen(response),
                0
            );

            break;
        }

        /* Unknown command -------------------------------- */

        const char *response =
            "ERR 006 UNKNOWN_COMMAND SID:1673\n";

        send(
            client_fd,
            response,
            strlen(response),
            0
        );
    }

    close(client_fd);

    return NULL;
}

/* ---------------------------------------------------------
   Main Agent
   --------------------------------------------------------- */
int main(void) {

    int server_fd;

    struct sockaddr_in server_addr;

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

    if (listen(server_fd, 10) < 0) {

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

        client_info->client_fd =
            accept(
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

            perror("pthread_create");

            close(client_info->client_fd);

            free(client_info);

            continue;
        }

        pthread_detach(thread_id);
    }

    close(server_fd);

    return 0;
}
