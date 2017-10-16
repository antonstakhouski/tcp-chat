/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#include "constants.h"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[MAX_LEN];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    //create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);

    //bind socket
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
                sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    //listen
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd,
            (struct sockaddr *) &cli_addr,
            &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");


    time_t rawtime;
    struct tm * timeinfo;
    while (1) {
        //read
        bzero(buffer,MAX_LEN);
        if ((n = read(newsockfd, buffer, MAX_LEN)) > 0) {
            // close
            if (!strncmp(buffer, CLOSE_STR, strlen(CLOSE_STR))) {
                close(newsockfd);
                close(sockfd);
                exit(0);
            }
            // echo
            if (!strncmp(buffer, ECHO_STR, strlen(ECHO_STR))) {
                puts(buffer);
                bzero(buffer, MAX_LEN);
                while(!read(newsockfd, buffer, MAX_LEN - 1));
                puts(buffer);
                while((n = read(newsockfd, buffer, MAX_LEN - 1)) > 0) {
                    puts("echo");
                    puts(buffer);
                    bzero(buffer, MAX_LEN);
                }
                if (n < 0)
                    error("ERROR writing to socket");
            }

            // upload
            if (!strncmp(buffer, UPLOAD_STR, strlen(UPLOAD_STR))) {
                puts(buffer);
                FILE* out_file = fopen("out", "wb");
                size_t sdiff = n - strlen(UPLOAD_STR);
                if (sdiff > (size_t)0) {
                    fwrite(buffer + strlen(UPLOAD_STR), sdiff, 1, out_file);
                }
                bzero(buffer, MAX_LEN);
                while((n = read(newsockfd, buffer, MAX_LEN)) > 0) {
                    fwrite(buffer, 1, n, out_file);
                    bzero(buffer, MAX_LEN);
                }
                fclose(out_file);
                if (n < 0)
                    error("ERROR writing to socket");
            }

            // time
            if (!strncmp(buffer, TIME_STR, strlen(TIME_STR))) {
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                n = write(newsockfd, asctime(timeinfo), strlen(asctime(timeinfo)));
                if (n < 0) error("ERROR writing to socket");
            }
        }
    }
    close(newsockfd);
    close(sockfd);
    return 0;
}
