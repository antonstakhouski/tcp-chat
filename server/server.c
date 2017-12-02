/* A simple server in the internet domain using TCP
   The port number is passed as an argument */

#include "../cross_header.h"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int echo(char* buff, int newsockfd)
{
    char buffer[MAX_LEN] = {0};

    if (strlen(buff))
        puts(buff);

    if (recv(newsockfd, buffer, MAX_LEN - 1, 0) < 0)
        return -1;
    if (send(newsockfd, buffer, strlen(buffer), 0) < 0)
        return -1;
    puts(buffer);
    return 0;
}

int upload(int newsockfd)
{
    char buffer[MAX_LEN] = {0};
    int n;
    char filename[256] = {0};
    char* size_pos;
    FILE* out_file;
    long filesize;
    size_t filename_len;
    fd_set set, set_error;
    struct timeval timeleft = {30, 0};
    int res;
    int bytes_received = 0;

    // if we get no info after command
    if ((n = recv(newsockfd, buffer, MAX_LEN, 0)) < 0) {
        printf("Error: %s", strerror(n));
        return -1;
    }
    bytes_received += n;
    size_pos = strstr(buffer, "\n");
    filename_len = size_pos - buffer;
    strncat(filename, buffer, filename_len);
    printf("Filename: %s\n", filename);
    printf("Packet length: %d\n", n);
    out_file = fopen(filename, "wb");
    size_pos++;

    filesize= *((long*)size_pos);
    printf("Filesize: %ld\n", filesize);
    size_t predata = size_pos + sizeof(filesize) - buffer;
    filesize -= fwrite(size_pos + sizeof(filesize), 1, n - predata, out_file);

    // read from socket to file
    while (filesize > 0) {
        FD_SET(newsockfd, &set);
        FD_SET(newsockfd, &set_error);
        if ((res = select( newsockfd + 1, &set, NULL, &set_error, &timeleft)) < 0) {
            printf("Error: %s", strerror(res));
        }

        if (FD_ISSET(newsockfd, &set_error)) {
            memset(buffer, 0, MAX_LEN);
            if ((n = recv(newsockfd, buffer, MAX_LEN, MSG_OOB)) < 0) {
                printf("Error: %s\n", strerror(n));
                return -1;
            }
            else{
                printf("Received %d bytes\n", bytes_received);
                printf("OOB data: %d\n", *((int *)buffer));
            }
        } else if (!FD_ISSET(newsockfd, &set)) {
            printf("Transfer error\n");
            close_sock(newsockfd);
            fclose(out_file);
            return -1;
        }

        memset(buffer, 0, MAX_LEN);
        if ((n = recv(newsockfd, buffer, MAX_LEN, 0)) < 0) {
            printf("Error: %s", strerror(n));
            return -1;
        }
        bytes_received += n;
        filesize -= fwrite(buffer, 1, n, out_file);
        FD_CLR(newsockfd, &set);
        FD_CLR(newsockfd, &set_error);
    }
    fclose(out_file);
    return 0;
}

int send_time(int newsockfd)
{
    time_t rawtime;
    struct tm * timeinfo;
    puts("Time sent");
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    if(send(newsockfd, asctime(timeinfo), strlen(asctime(timeinfo)), 0) < 0)
        return -1;
    return 0;
}

int main(int argc, char *argv[])
{
    init();
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

    set_keepalive(sockfd);

    //bind socket
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    assert(!bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));

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
                if (n < 0)
                    break;
                if (!strncmp(buffer, CLOSE_STR, strlen(CLOSE_STR))) {
                    if (close_sock(newsockfd) < 0)
                        break;
                    break;
                }
                if (!strncmp(buffer, ECHO_STR, strlen(ECHO_STR)))
                    if (echo(strstr(buffer, "\n") + 1, newsockfd) < 0)
                        break;
                if (!strncmp(buffer, UPLOAD_STR, strlen(UPLOAD_STR))) {
                    if(upload(newsockfd) < 0)
                        break;;
                }
                if (!strncmp(buffer, TIME_STR, strlen(TIME_STR)))
                    if (send_time(newsockfd) < 0 )
                        break;
            }
        }
    }
    close_sock(newsockfd);
    close_sock(sockfd);
    clear();
    return 0;
}
