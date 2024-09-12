#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define TCP_PORT 5100
#define BUF_SIZE 1024

void error_handling(const char *message);

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    char msg[BUF_SIZE];
    int str_len;

    if (argc != 2) {
        printf("Usage : %s <IP>\n", argv[0]);
        return -1;
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
    serv_addr.sin_port = htons(TCP_PORT);
    
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    pid_t pid = fork();
    if (pid == 0) {  // 자식 프로세스: 서버에서 메시지 수신
        while (1) {
            str_len = read(sock, msg, BUF_SIZE - 1);
            if (str_len <= 0)
                break;
            msg[str_len] = 0;
            printf("Message from server: %s", msg);
        }
        close(sock);
        return 0;
    } else {  // 부모 프로세스: 서버로 메시지 전송
        while (1) {
            fgets(msg, BUF_SIZE, stdin);
            if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")) {
                close(sock);
                break;
            }
            write(sock, msg, strlen(msg));
        }
    }
    return 0;
}

void error_handling(const char *message) {
    perror(message);
    exit(1);
}
