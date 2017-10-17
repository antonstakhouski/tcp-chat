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
#include <stropts.h>

#include "../constants.h"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void echo(char* buff, int newsockfd)
{
    char buffer[MAX_LEN] = {0};
    puts("echo");

    if (strlen(buff))
        puts(buff);

    read(newsockfd, buffer, MAX_LEN - 1);
    puts(buffer);
}

void upload(int newsockfd)
{
    char buffer[MAX_LEN] = {0};
    int n;
    char filename[256] = {0};
    char* size_pos;
    FILE* out_file;
    long filesize;
    size_t filename_len;

    // if we get no info after command
    n = read(newsockfd, buffer, MAX_LEN);
    printf("n = %d\n", n);
    size_pos = strstr(buffer, "\n");
    while(!size_pos) {
        strcat(filename, buffer);
        n = read(newsockfd, buffer, MAX_LEN);
        size_pos = strstr(buffer, "\n");
    }
    filename_len = size_pos - buffer;
    strncat(filename, buffer, filename_len);
    out_file = fopen(filename, "wb");
    size_pos++;

    filesize= *((long*)size_pos);
    if (!filesize) {
        puts("solo name");
        bzero(buffer, MAX_LEN);
        n = read(newsockfd, buffer, MAX_LEN);
        filesize= *((long*)buffer);
        filesize -= fwrite(buffer + sizeof(filesize), 1, n - sizeof(filesize), out_file);
    }
    else {
        puts("name + stuff");
        size_t predata = size_pos + sizeof(filesize) - buffer;
        filesize -= fwrite(size_pos + sizeof(filesize), 1, n - predata, out_file);
    }
    /** size_t length_pos = strstr(buffer, "\n") + 1; */

    /** if (n == sizeof(filesize)) { */
    /** char* length_pos = strstr(buffer, "\n") + 1; */
    /** filesize= *((long*)buffer); */
    /** n = read(newsockfd, buffer, MAX_LEN); */
    /** text_pos = strstr(buffer, "\n") + 1; */
    /** name_len = text_pos - buffer - 1; */
    /** strncpy(filename, buffer, name_len); */
    /** filesize -= fwrite(text_pos, 1, n - name_len - 1, out_file); */
    /** } */
    /** else { */
    /**     puts("fsize + stuff"); */
    /**     filesize= *((long*)buffer); */
    /**     printf("filesize %zu\n", filesize); */
    /**     text_pos = strstr(buffer, "\n") + 1; */
    /**     name_len = text_pos - buffer - 1 - sizeof(filename); */
    /**     strncpy(filename, buffer + sizeof(filename), name_len); */
    /**     out_file = fopen(filename, "wb"); */
    /**     filesize -= fwrite(text_pos, 1, n - name_len - 1 - sizeof(filename), out_file); */
    /**     bzero(buffer, MAX_LEN); */
    /** } */

    puts("ok");
    // read from socket to file
    while (filesize) {
        n = read(newsockfd, buffer, MAX_LEN);
        filesize -= fwrite(buffer, 1, n, out_file);
        bzero(buffer, MAX_LEN);
    }
    fclose(out_file);
}

void send_time(int newsockfd)
{
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    write(newsockfd, asctime(timeinfo), strlen(asctime(timeinfo)));
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
                    echo(strstr(buffer, "\n") + 1, newsockfd);
                if (!strncmp(buffer, UPLOAD_STR, strlen(UPLOAD_STR))) {
                    upload(newsockfd);
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
