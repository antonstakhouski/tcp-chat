#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>

#include "../constants.h"

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void get_cmd(int opcode, char* cmd)
{
    switch(opcode) {
        case 1:
            strcpy(cmd, TIME_STR);
            break;
        case 2:
            strcpy(cmd, ECHO_STR);
            break;
        case 3:
            strcpy(cmd, UPLOAD_STR);
            break;
        case 0:
            strcpy(cmd, CLOSE_STR);
    }
}

void get_time(int sockfd)
{
    char buffer[MAX_LEN] = {0};
    read(sockfd, buffer, MAX_LEN);
    printf("%s\n", buffer);
}

void echo(int sockfd)
{
    char buffer[MAX_LEN];
    scanf("%s", buffer);
    write(sockfd, buffer, strlen(buffer));
}

void upload(char* filename, int sockfd)
{
    FILE* file;
    char buffer[MAX_LEN];

    if(!(file = fopen(filename, "rb"))) {
        puts("not open");
        exit(1);
    }
    size_t size = 0;
    bzero(buffer, MAX_LEN);
    strcpy(buffer, filename);
    strcat(buffer, "\n");
    write(sockfd, buffer, strlen(buffer));

    bzero(buffer, MAX_LEN);
    while (( size = fread(buffer, 1, MAX_LEN, file))) {
        write(sockfd, buffer, size);
        bzero(buffer, MAX_LEN);
    }
    fclose(file);
}

void show_help()
{
    puts("Choose action:");
    puts("1 - TIME");
    puts("2 - ECHO");
    puts("3 - UPLOAD");
    puts("0 - EXIT");
}

int main(int argc, char *argv[])
{
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 3) {
       fprintf(stderr, "usage %s hostname port\n", argv[0]);
       exit(0);
    }

    //create socket
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    //bind socket
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    int choize;
    char cmd[256];
    while (1) {
        show_help();

        int res;
        res = scanf("%d", &choize);
        while(!res || choize < 0 || choize > 3)
            res = scanf("%d", &choize);
        get_cmd(choize, cmd);
        write(sockfd, cmd, strlen(cmd));

        if (!strncmp(cmd, TIME_STR, strlen(TIME_STR) - 1))
            get_time(sockfd);

        if (!strncmp(cmd, ECHO_STR, strlen(ECHO_STR) - 1)) {
            echo(sockfd);
        }

        if (!strncmp(cmd, UPLOAD_STR, strlen(UPLOAD_STR) - 1)) {
            char filename[256];
            puts("Enter filename");
            scanf("%s", filename);
            upload(filename, sockfd);
        }

        if (!strncmp(cmd, CLOSE_STR, strlen(CLOSE_STR) - 1)) {
            close(sockfd);
            return 0;
        }
    }

    return 0;
}
