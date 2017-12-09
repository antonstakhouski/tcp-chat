#include "server_lib.h"

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

int udp_upload(int newsockfd)
{
    char buffer[MAX_LEN] = {0};
    int n;
    char filename[256] = {0};
    char* size_pos;
    FILE* out_file;
    long filesize;
    size_t filename_len;
    int bytes_received = 0;
    unsigned int fromlen = sizeof(struct sockaddr_in);
    struct sockaddr_in from;

    // if we get no info after command
    if ((n = recvfrom(newsockfd, buffer, MAX_LEN, 0,
                    (struct sockaddr*)&from, &fromlen)) < 0) {
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

    int sn = 0;
    int an = 0;
    int ack_size;
    // read from socket to file
    while (filesize > 0) {
        memset(buffer, 0, MAX_LEN);
        if ((n = recvfrom(newsockfd, buffer, MAX_LEN, 0,
                        (struct sockaddr*)&from, &fromlen)) < 0) {
            printf("Error: %s", strerror(n));
            return -1;
        }
        sn = *((int*)buffer);
        if (sn == an) {
            ack_size  = sendto(newsockfd, &sn, sizeof(sn), 0,
                    (struct sockaddr *)&from, fromlen);
            if (ack_size < 0) error("recvfrom");
            filesize -= fwrite(buffer + sizeof(sn), 1, n - sizeof(sn), out_file);
            an++;
        } else {
            puts("Transfer error");
        }
        bytes_received += n;
    }
    fclose(out_file);
    puts("File received");
    return 0;
}

int tcp_upload(int newsockfd)
{
    char buffer[MAX_LEN] = {0};
    int n;
    char filename[256] = {0};
    char* size_pos;
    FILE* out_file;
    int64_t filesize;
    size_t filename_len;
    long bytes_received = 0;

    // if we get no info after command
    if ((n = recv(newsockfd, buffer, MAX_LEN, 0)) < 0) {
        printf("Error: %s in line %d\n", strerror(n), __LINE__);
        return -1;
    }
    bytes_received += n;
    size_pos = strstr(buffer, "\n");
    filename_len = size_pos - buffer;
    printf("Filename len: %zu\n", filename_len);
    strncat(filename, buffer, filename_len);
    printf("Filename: %s\n", filename);
    printf("Packet length: %d\n", n);
    out_file = fopen(filename, "wb");
    size_pos++;
    printf("Filesize takes %zu bytes\n", sizeof(filesize));

    filesize = *((long*)size_pos);
    printf("Filesize: %" PRId64 "\n", filesize);
    size_t predata = size_pos + sizeof(filesize) - buffer;
    filesize -= fwrite(size_pos + sizeof(filesize), 1, n - predata, out_file);

    fd_set set, set_error;
    int res;
    struct timeval timeleft = {30, 0};
    // read from socket to file
    while (filesize > 0) {
        FD_SET(newsockfd, &set);
        FD_SET(newsockfd, &set_error);
        if ((res = select( newsockfd + 1, &set, NULL, &set_error, &timeleft)) < 0) {
            printf("Error: %s in line %d\n", strerror(n), __LINE__);
        }

        if (FD_ISSET(newsockfd, &set_error)) {
            memset(buffer, 0, MAX_LEN);
            if ((n = recv(newsockfd, buffer, MAX_LEN, MSG_OOB)) < 0) {
                printf("Error: %s in line %d\n", strerror(n), __LINE__);
                return -1;
            }
            else{
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
            printf("Error: %s in line %d\n", strerror(n), __LINE__);
            return -1;
        }
        /*printf("Packet length: %d\n", n);*/
        bytes_received += n;
        filesize -= fwrite(buffer, 1, n, out_file);
    }
    fclose(out_file);
    printf("%ld bytes received\n", bytes_received);
    puts("File received");
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

void udp_loop(int sockfd)
{
    char buffer[MAX_LEN];
    struct sockaddr_in from;
    int n;
    socklen_t fromlen;
    fromlen = sizeof(struct sockaddr_in);

    puts("Server is ready");
    while (1) {
        memset(buffer, 0, MAX_LEN);
        if ((n = recvfrom(sockfd, buffer, MAX_LEN, 0,
                        (struct sockaddr *) &from,  &fromlen)) >= 0) {
            if (!strncmp(buffer, UPLOAD_STR, strlen(UPLOAD_STR))) {
                if(udp_upload(sockfd) < 0)
                    break;
            }
        } else {
            printf("Error recvfrom\n");
        }
    }
}

void tcp_loop(int sockfd)
{
    char buffer[MAX_LEN];
    struct sockaddr_in cli_addr;
    socketlen clilen;

    //listen
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    int newsockfd, n;

    puts("Server is ready");
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
                    if(tcp_upload(newsockfd) < 0)
                        break;
                }
                if (!strncmp(buffer, TIME_STR, strlen(TIME_STR)))
                    if (send_time(newsockfd) < 0 )
                        break;
            }
        }
    }
    close_sock(newsockfd);
}
