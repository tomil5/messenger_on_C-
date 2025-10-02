#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 5000
#define BUF_SIZE 1024
#define MAX_CLIENTS 10
#define NAME_LEN 32

typedef struct {
    int sock;
    char name[NAME_LEN];
    int named; // 0 - имя ещё не введено, 1 - уже есть
} Client;

int main() {
    int server_fd, client_fd, max_sd, activity;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUF_SIZE];
    fd_set readfds;

    Client clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = 0;
        clients[i].named = 0;
    }

    // создаём сокет
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket"); exit(1);
    }

    // привязываем сокет
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); exit(1);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen"); exit(1);
    }

    printf("Chat server started on port %d\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].sock;
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        // новое подключение
        if (FD_ISSET(server_fd, &readfds)) {
            client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            printf("New client connected: %d\n", client_fd);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].sock == 0) {
                    clients[i].sock = client_fd;
                    clients[i].named = 0;
                    send(client_fd, "Enter your name: ", 17, 0);
                    break;
                }
            }
        }

        // обработка сообщений
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].sock;
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                int n = read(sd, buffer, BUF_SIZE);
                if (n <= 0) {
                    printf("Client disconnected: %d\n", sd);
                    close(sd);
                    clients[i].sock = 0;
                    clients[i].named = 0;
                } else {
                    buffer[n] = '\0';

                    if (!clients[i].named) {
                        strncpy(clients[i].name, buffer, NAME_LEN-1);
                        clients[i].name[strcspn(clients[i].name, "\n")] = '\0'; // убрать \n
                        clients[i].named = 1;

                        char msg[BUF_SIZE];
                        snprintf(msg, BUF_SIZE, "[SERVER] %s joined the chat\n", clients[i].name);

                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j].sock > 0) send(clients[j].sock, msg, strlen(msg), 0);
                        }
                    } else {
                        char msg[BUF_SIZE];
                        snprintf(msg, BUF_SIZE, "[%s] %s", clients[i].name, buffer);

                        printf("%s", msg);

                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j].sock > 0 && j != i) {
                                send(clients[j].sock, msg, strlen(msg), 0);
                            }
                        }
                    }
                }
            }
        }
    }
}
