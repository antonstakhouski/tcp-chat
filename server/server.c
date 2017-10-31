/* A simple server in the internet domain using TCP
   The port number is passed as an argument */

#include "../cross_header.h"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void echo(char* buff, int newsockfd)
{
    char buffer[MAX_LEN] = {0};

    if (strlen(buff))
        puts(buff);

    recv(newsockfd, buffer, MAX_LEN - 1, 0);
    puts(buffer);
}

void upload(char* fname, size_t bsize, int newsockfd)
{
    char buffer[MAX_LEN] = {0};
    int n;
    char filename[256] = {0};
    char* size_pos;
    FILE* out_file;
    long filesize;
    size_t filename_len;

    if (strlen(fname)) {
        size_pos = strstr(fname, "\n");
        while(!size_pos) {
            strcat(filename, fname);
            n = recv(newsockfd, fname, MAX_LEN, 0);
            size_pos = strstr(fname, "\n");
        }
        filename_len = size_pos - fname;
        strncat(filename, fname, filename_len);
        out_file = fopen(filename, "wb");
        size_pos++;

        filesize= *((long*)size_pos);
        if (!filesize) {
            memset(buffer, 0, MAX_LEN);
            n = recv(newsockfd, buffer, MAX_LEN, 0);
            filesize= *((long*)buffer);
            filesize -= fwrite(buffer + sizeof(filesize),
                    1, n - sizeof(filesize), out_file);
        }
        else {
            size_t predata = size_pos + sizeof(filesize) - fname;
            filesize -= fwrite(size_pos + sizeof(filesize), 1, bsize - predata, out_file);
        }
    }
    else {
        // if we get no info after command
        n = recv(newsockfd, buffer, MAX_LEN, 0);
        size_pos = strstr(buffer, "\n");
        while(!size_pos) {
            strcat(filename, buffer);
            n = recv(newsockfd, buffer, MAX_LEN, 0);
            size_pos = strstr(buffer, "\n");
        }
        filename_len = size_pos - buffer;
        strncat(filename, buffer, filename_len);
        out_file = fopen(filename, "wb");
        size_pos++;

        filesize= *((long*)size_pos);
        if (!filesize) {
            memset(buffer, 0, MAX_LEN);
            n = recv(newsockfd, buffer, MAX_LEN, 0);
            filesize= *((long*)buffer);
            filesize -= fwrite(buffer + sizeof(filesize),
                    1, n - sizeof(filesize), out_file);
        }
        else {
            size_t predata = size_pos + sizeof(filesize) - buffer;
            filesize -= fwrite(size_pos + sizeof(filesize), 1, n - predata, out_file);
        }
    }
    // read from socket to file
    while (filesize) {
        n = recv(newsockfd, buffer, MAX_LEN, 0);
        filesize -= fwrite(buffer, 1, n, out_file);
        memset(buffer, 0, MAX_LEN);
    }
    fclose(out_file);
}

void send_time(int newsockfd)
{
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    send(newsockfd, asctime(timeinfo), strlen(asctime(timeinfo)), 0);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socketlen clilen;
    char buffer[MAX_LEN];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    //create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    portno = atoi(argv[1]);

    /** int optval; */
    /** socklen_t optlen = sizeof(optval); */
    /**  */
    /* Set the option active */
    /** optval = 1; */
    /** optlen = sizeof(optval); */
    /** if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) { */
    /**     perror("setsockopt()"); */
    /**     close(sockfd); */
    /**     exit(EXIT_FAILURE); */
    /** } */
    /**  */
    /** optval = 3; */
    /* Check the status for the keepalive option */
    /** if(getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &optval, &optlen) < 0) { */
    /**     perror("getsockopt()"); */
    /**     close(sockfd); */
    /**     exit(EXIT_FAILURE); */
    /** } */
    /**  */
    /** optval = 5; */
    /* Check the status for the keepalive option */
    /** if(getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &optval, &optlen) < 0) { */
    /**     perror("getsockopt()"); */
    /**     close(sockfd); */
    /**     exit(EXIT_FAILURE); */
    /** } */
    /**  */
    /** optval = 2; */
    /* Check the status for the keepalive option */
    /** if(getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &optval, &optlen) < 0) { */
    /**     perror("getsockopt()"); */
    /**     close(sockfd); */
    /**     exit(EXIT_FAILURE); */
    /** } */
    /**  */
    //bind socket
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    bind(sockfd, (struct sockaddr *) &serv_addr,
                sizeof(serv_addr));

    //listen
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    while (1) {
        newsockfd = accept(sockfd,
                (struct sockaddr *) &cli_addr,
                &clilen);

        while (1) {
            memset(buffer, 0, MAX_LEN);
            if ((n = recv(newsockfd, buffer, MAX_LEN, 0)) > 0) {
                if (!strncmp(buffer, CLOSE_STR, strlen(CLOSE_STR))) {
                    close_sock(newsockfd);
                    break;
                }
                if (!strncmp(buffer, ECHO_STR, strlen(ECHO_STR)))
                    echo(strstr(buffer, "\n") + 1, newsockfd);
                if (!strncmp(buffer, UPLOAD_STR, strlen(UPLOAD_STR))) {
                    char* fname = strstr(buffer, "\n") + 1;
                    upload(fname, n - (fname - buffer), newsockfd);
                }
                if (!strncmp(buffer, TIME_STR, strlen(TIME_STR)))
                    send_time(newsockfd);
            }
            /** else  { */
            /**     printf("%s\n", strerror(errno)); */
            /**     exit(0); */
            /** } */
        }
    }
    close_sock(newsockfd);
    close_sock(sockfd);
    return 0;
}
