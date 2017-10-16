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
#include <assert.h>

#include "../constants.h"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void echo(char* buffer, int newsockfd)
{
    int n;
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

void upload(char* buff, int bsize, int newsockfd)
{
    char buffer[MAX_LEN] = {0};
    int n;
    char filename[256] = {0};
    char* text_pos;
    FILE* out_file;
    size_t name_len;

    if (strlen(buff)) {
        // if we get some stuff with command
        text_pos = strstr(buff, "\n") + 1;
        name_len = text_pos - buff - 1;
        strncpy(filename, buff, name_len);
        out_file = fopen(filename, "wb");
        fwrite(text_pos, 1, bsize - name_len - 1, out_file);
    }
    else {
        // if we get no info after command
        n = read(newsockfd, buffer, MAX_LEN);
        text_pos = strstr(buffer, "\n") + 1;
        name_len = text_pos - buffer - 1;
        strncpy(filename, buffer, name_len);
        out_file = fopen(filename, "wb");
        fwrite(text_pos, 1, n - name_len - 1, out_file);
        bzero(buffer, MAX_LEN);
    }

    // read from socket to file
    while((n = read(newsockfd, buffer, MAX_LEN)) > 0) {
        fwrite(buffer, 1, n, out_file);
        bzero(buffer, MAX_LEN);
    }
    fclose(out_file);
}

void send_time(int newsockfd)
{
    time_t rawtime;
    struct tm * timeinfo;
    int n;
    // time
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    n = write(newsockfd, asctime(timeinfo), strlen(asctime(timeinfo)));
    if (n < 0) error("ERROR writing to socket");
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
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    while (1) {
        newsockfd = accept(sockfd,
                (struct sockaddr *) &cli_addr,
                &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        while (1) {
            bzero(buffer,MAX_LEN);
            if ((n = read(newsockfd, buffer, MAX_LEN)) > 0) {
                if (!strncmp(buffer, CLOSE_STR, strlen(CLOSE_STR))) {
                    close(newsockfd);
                    break;
                }
                if (!strncmp(buffer, ECHO_STR, strlen(ECHO_STR)))
                    echo(buffer, newsockfd);
                if (!strncmp(buffer, UPLOAD_STR, strlen(UPLOAD_STR))) {
                    upload(strstr(buffer, "\n") + 1, n - strlen(UPLOAD_STR), newsockfd);
                }
                if (!strncmp(buffer, TIME_STR, strlen(TIME_STR)))
                    send_time(newsockfd);
            }
        }
    }
    close(newsockfd);
    close(sockfd);
    return 0;
}
