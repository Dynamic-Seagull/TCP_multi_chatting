#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define TCP_PORT 5100
#define BUFSIZE 1024

int main(int argc, char *argv[]) 
{
    int sock;
    struct sockaddr_in saddr;
    char msg[BUFSIZE];
    int slen;

    if (argc != 2) 
    {
        printf("Usage : %s <IP>\n", argv[0]);
        return -1;
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        {
            perror("socket() error");
            exit(1);
        }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &saddr.sin_addr);
    saddr.sin_port = htons(TCP_PORT);
    
    if (connect(sock, (struct sockaddr*)&saddr, sizeof(saddr)) == -1)
    {
        perror("connect() error");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == 0) 
    {  // 자식 프로세스 - 서버에서 메시지 수신
        while (1) 
        {
            slen = read(sock, msg, BUFSIZE - 1);
            if (slen <= 0)
                break;
            msg[slen] = '\0';
            printf("%s\n", msg);
        }
        close(sock);
        return 0;
    } 
    else 
    {  // 부모 프로세스 - 서버로 메시지 전송
        while (1) 
        {
            fgets(msg, BUFSIZE, stdin);
            if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")) 
            {
                close(sock);
                break;
            }
            write(sock, msg, strlen(msg));
        }
    }
    return 0;
}
