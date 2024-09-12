#include "server.h"            

int login(char idpw[])
{
    //char check[100];
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
    return 0;
}

void regist()
{
    char nid[20];
    char npw[100];

    FILE* fp;
    fp = fopen("userList.txt", "a");
    if (fp == NULL)
    {
        printf("userList Not found.\n");
        return;
    }

    while (1)
    {
        printf("Input id: ");
        fgets(nid, 20, stdin);
        nid[strlen(nid) - 1] = '\0';
        if ((strcmp(nid, "new") == 0))
        {
            printf("'new' is Not available.");
            continue;
        }
        printf("Input password: ");
        fgets(npw, 100, stdin);
        npw[strlen(npw) - 1] = '\0';
        strcat(npw, nid);
        fputs(npw, fp);
        fputs("\n", fp);
        break;
    }

    fclose(fp);
    return;
}
