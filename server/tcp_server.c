#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define TCP_PORT 5100
#define BUF_SIZE 1024
#define MAX_CLIENTS 50

int client_pipes[MAX_CLIENTS][2];  // 파이프 배열 (클라이언트 수만큼 생성)
int client_socks[MAX_CLIENTS];     // 클라이언트 소켓 배열
int client_count = 0;              // 현재 접속한 클라이언트 수

void error_handling(const char *message) {
    perror(message);
    exit(1);
}

void handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void broadcast_message(int sender_index, char *message) {
    // 모든 클라이언트에게 메시지 전송 (sender_index는 송신자 인덱스, 송신자 제외)
    for (int i = 0; i < client_count; i++) {
        if (i != sender_index) {  // 송신자 제외
            write(client_socks[i], message, strlen(message));
        }
    }
}

void daemonize() {
    pid_t pid;

    // 1. 부모 프로세스 종료
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);  // 부모 프로세스 종료
    }

    // 2. 세션 리더가 되기
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    // 3. 표준 입출력 닫기
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // 4. 파일 디스크립터 리다이렉트 (로그 파일로 연결해도 됨)
    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) {
            close(fd);
        }
    }

    // 5. 루트 디렉토리로 이동
    if (chdir("/") < 0) {
        exit(EXIT_FAILURE);
    }

    // 6. 파일 권한 마스크 설정
    umask(0);
}

int main() {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    struct sigaction act;
    char msg[BUF_SIZE];
    int i, n;

    // SIGCHLD 시그널 처리 (자식 프로세스 종료 처리)
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, 0);

    // 서버 데몬화
    daemonize();

    // 서버 소켓 생성
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(TCP_PORT);

    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while (1) {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

        if (clnt_sock == -1)
            continue;

        if (client_count >= MAX_CLIENTS) {
            printf("Max clients reached. Rejecting connection.\n");
            close(clnt_sock);
            continue;
        }

        // 파이프 생성
        if (pipe(client_pipes[client_count]) == -1) {
            perror("pipe() error");
            exit(1);
        }

        client_socks[client_count] = clnt_sock;  // 클라이언트 소켓 저장

        pid_t pid = fork();
        if (pid == 0) {  // 자식 프로세스: 클라이언트와 통신
            close(serv_sock);

            // 파이프의 읽기 끝을 닫고, 클라이언트 소켓을 통해 수신한 데이터를 파이프로 전송
            close(client_pipes[client_count][0]);

            while ((n = read(clnt_sock, msg, BUF_SIZE)) > 0) {
                msg[n] = '\0';
                printf("Received from client %d: %s", client_count, msg);

                // 클라이언트의 메시지를 파이프로 서버 프로세스로 전송
                write(client_pipes[client_count][1], msg, strlen(msg));
            }

            close(clnt_sock);
            close(client_pipes[client_count][1]);
            exit(0);
        }

        client_count++;  // 클라이언트 수 증가

        // 부모 프로세스: 파이프에서 메시지 읽고 브로드캐스트
        for (i = 0; i < client_count; i++) {
            if (fork() == 0) {  // 자식 프로세스: 해당 클라이언트의 파이프 읽기
                char buffer[BUF_SIZE];

                close(client_pipes[i][1]);  // 쓰기 끝 닫기

                while ((n = read(client_pipes[i][0], buffer, BUF_SIZE)) > 0) {
                    buffer[n] = '\0';
                    printf("Broadcasting message from client %d: %s", i, buffer);
                    
                    // 브로드캐스트 메시지 전송 (해당 클라이언트 제외)
                    broadcast_message(i, buffer);
                }

                close(client_pipes[i][0]);
                exit(0);
            }
        }
    }
    close(serv_sock);
    return 0;
}
