#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>

#include "constants.h"

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[MAX_LEN];
    if (argc < 4) {
       fprintf(stderr, "usage %s hostname port command [file]\n", argv[0]);
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

    bzero(buffer, MAX_LEN);
    strcpy(buffer, argv[3]);
    strcat(buffer, "\n");
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket");
    // get time
    if (!strncmp(argv[3], TIME_STR, strlen(TIME_STR) - 1)) {
        bzero(buffer, MAX_LEN);
        n = read(sockfd, buffer, MAX_LEN - 1);
        if (n < 0)
            error("ERROR reading from socket");
        printf("%s\n", buffer);
    }
    // echo
    if (!strncmp(argv[3], ECHO_STR, strlen(ECHO_STR) - 1)) {
        if (argc >= 5) {
            size_t i = 0;
            puts(argv[4]);
            while (i < strlen(argv[4])) {
                bzero(buffer, MAX_LEN);
                strncpy(buffer, &(argv[4][i]), MAX_LEN - 1);
                n = write(sockfd, buffer, MAX_LEN);
                if (n < 0)
                    error("ERROR writing to socket");
                i += (MAX_LEN - 1);
            }
        }
    }
    // upload
    if (!strncmp(argv[3], UPLOAD_STR, strlen(UPLOAD_STR) - 1)) {
        FILE* file;
        if(!(file = fopen(argv[4], "rb"))) {
            puts("not open");
            exit(1);
        }
        size_t size = 0;
        if (argc >= 5) {
            bzero(buffer, MAX_LEN);
            strcpy(buffer, argv[4]);
            strcat(buffer, "\n");
            printf("%zu", strlen(buffer));
            write(sockfd, buffer, strlen(buffer));

            bzero(buffer, MAX_LEN);
            while (( size = fread(buffer, 1, MAX_LEN, file))) {
                n = write(sockfd, buffer, size);
                bzero(buffer, MAX_LEN);
                if (n < 0)
                    error("ERROR writing to socket");
            }
        }
        fclose(file);
    }
    close(sockfd);
    return 0;
}
