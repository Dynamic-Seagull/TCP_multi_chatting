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
#include <sys/types.h>
#include <termios.h>

#define TCP_PORT 5100
#define BUFSIZE 1024
#define MAX_CLIENTS 50

int client_pipes[MAX_CLIENTS][2];  // 파이프 배열
int client_socks[MAX_CLIENTS];
int client_count = 0;

int login(char idpw[])
{
    // 회원여부 확인
    // id와 pw가 합쳐진 문자열이 txt 파일에 저장되어있음 (pw+id 순서)
    FILE* fp;
    fp = fopen("userList.txt", "r");
    if (fp == NULL)
    {
        printf("userList Not found.\n");
        return 0;
    }
    printf("id check start\n");
    while (!feof(fp))
    {
        char check[100];
        char *ps = fgets(check, sizeof(check), fp);
        check[strlen(check) - 1] = '\0';
        if (!(strcmp(check, idpw)))
        {
            printf("login success!\n");
            fclose(fp);
            return 0;
        }
    }
    printf("login fail\n");

    fclose(fp);
    return -1;
}

void rgst(char nidpw[])
{
    //회원 등록
    FILE* fp;
    fp = fopen("userList.txt", "a");
    if (fp == NULL)
    {
        printf("userList Not found.\n");
        return;
    }
    
    fputs(nidpw, fp);
    fputs("\n", fp);
    fclose(fp);
}

void handler(int signo) 
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void broadcast_message(int sender_index, char *message, char *id) 
{
    //발신자 제외 모든 클라이언트에게 메시지 전송
    for (int i = 0; i < client_count; i++) 
    {
        if (i != sender_index) 
        {
            write(client_socks[i], message, strlen(message));
        }
    }
}

void daemonize() //데몬 프로세스
{
    pid_t pid;

    pid = fork();
    if (pid < 0) 
        exit(EXIT_FAILURE);
    if (pid > 0) 
        exit(EXIT_SUCCESS);

    if (setsid() < 0) 
        exit(EXIT_FAILURE);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) 
    {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) 
            close(fd);
    }

    if (chdir("/") < 0) 
        exit(EXIT_FAILURE);

    umask(0);
}

int main() 
{
    int ssock, csock;
    struct sockaddr_in saddr, caddr;
    socklen_t caddr_sz;
    struct sigaction act;
    char msg[BUFSIZE];
    int i, n;
    char id[20];
    char pw[100];
    char nid[20];
    char npw[100];

    // 자식프로세스 종료 시그널
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, 0);

    // 서버 데몬화
    daemonize();

    // 서버 소켓 생성
    ssock = socket(PF_INET, SOCK_STREAM, 0);
    if (ssock == -1)
    {
        perror("socket() error");
        exit(1);
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(TCP_PORT);

    // bind() 오류 방지
    int yes = 1;
    if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
    {
        perror("setsockopt");
        return 1;
    } 

    if (bind(ssock, (struct sockaddr*)&saddr, sizeof(saddr)) == -1)
    {
        perror("bind() error");
        exit(1);
    }

    if (listen(ssock, 5) == -1)
    {
        perror("listen() error");
        exit(1);
    }

    while (1) 
    {
        caddr_sz = sizeof(caddr);
        csock = accept(ssock, (struct sockaddr*)&caddr, &caddr_sz);

        if (csock == -1)
            continue;

        if (client_count >= MAX_CLIENTS) 
        {
            printf("Max clients reached. Rejecting connection.\n");
            close(csock);
            continue;
        }

        printf("New client connected: %d\n", client_count + 1);

        // 파이프 생성
        if (pipe(client_pipes[client_count]) == -1) 
        {
            perror("pipe() error");
            exit(1);
        }

        client_socks[client_count] = csock;

        pid_t pid = fork();
        if (pid == 0) 
        {
            close(ssock);

            while(1)
            {
                // 로그인 처리
                write(client_socks[client_count], "ID (join - input 'new') : ", strlen("ID (join - input 'new') : "));
                if ((n = read(csock, id, 20)) <= 0) 
                {
                    perror("Input your ID");
                    close(client_socks[client_count]);
                    exit(1);
                }
                id[n - 1] = '\0';

                if (!(strcmp( id, "new")))
                {
                    // new 입력시 회원가입
                    write(client_socks[client_count], "Input ID : ", strlen("Input ID : "));
                    if ((n = read(csock, nid, 20)) <= 0) 
                    {
                        perror("Input your ID");
                        close(client_socks[client_count]);
                        exit(1);
                    }
                    printf("%s\n",nid);
                    if ((strcmp(nid, "new") == 0))
                    {
                        printf("'new' is Not available.");
                        exit(1);
                    }
                    nid[n - 1] = '\0';
                    write(client_socks[client_count], "Password : ", strlen("Password : "));
                    if ((n = read(csock, npw, 100)) <= 0) 
                    {
                        perror("Input your PW");
                        close(client_socks[client_count]);
                        exit(1);
                    }
                    npw[n - 1] = '\0';
                    strcat(npw, nid);
                    rgst(npw);
                    write(client_socks[client_count], "please login again! \n", strlen("please login again! \n"));
                    
                    continue;
                }

                write(client_socks[client_count], "Password : ", strlen("Password : "));
                if ((n = read(csock, pw, 100)) <= 0) 
                {
                    perror("read() password");
                    close(client_socks[client_count]);
                    exit(0);
                }
                pw[n - 1] = '\0';

                strcat(pw, id);
                if (login(pw) == 0) 
                {
                    write(client_socks[client_count], "Login successful!\nIf you want quit, Input 'q' or ' Q'\n", 
                                    strlen("Login successful!\nIf you want quit, Input 'q' or ' Q'\n"));
                    break;
                } else 
                {
                    write(client_socks[client_count], "Login failed!\n", strlen("Login failed!\n"));
                }
            }
            
        
            // 파이프 통신
            close(client_pipes[client_count][0]);

            while ((n = read(csock, msg, BUFSIZE)) > 0) 
            {
                msg[n] = '\0';
                printf("client %d [ %s ] : %s", client_count, id, msg);
                
                char setting1[1000] = "[ ";
                char setting2[10] = " ] ";
                strcat(setting1, id);
                strcat(setting1, setting2);
                strcat(setting1, msg);
                strcpy(msg, setting1);

                // 클라이언트 -> 서버
                write(client_pipes[client_count][1], msg, strlen(msg));
            }

            close(csock);
            close(client_pipes[client_count][1]);
            exit(0);
        }

        client_count++;

        // 부모 프로세스
        for (i = 0; i < client_count; i++) 
        {
            if (fork() == 0) 
            { 
                char buffer[BUFSIZE];

                close(client_pipes[i][1]);

                // 메세지 브로드캐스트
                while ((n = read(client_pipes[i][0], buffer, BUFSIZE)) > 0) 
                {
                    buffer[n] = '\0';
                    broadcast_message(i, buffer, id);
                }

                close(client_pipes[i][0]);
                exit(0);
            }
        }
    }
    close(ssock);
    return 0;
}
