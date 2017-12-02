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

    // read from socket to file
    while (filesize > 0) {
        memset(buffer, 0, MAX_LEN);
        if ((n = recvfrom(newsockfd, buffer, MAX_LEN, 0,
                        (struct sockaddr*)&from, &fromlen)) < 0) {
            printf("Error: %s", strerror(n));
            return -1;
        }
        bytes_received += n;
        filesize -= fwrite(buffer, 1, n, out_file);
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
    int sockfd, portno;
    char buffer[MAX_LEN];
    struct sockaddr_in serv_addr, from;
    int n;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    //create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(sockfd >= 0);

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    portno = atoi(argv[1]);

    //bind socket
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    assert(!bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));

    socklen_t fromlen;
    fromlen = sizeof(struct sockaddr_in);
    puts("Server is ready");

    while (1) {
        memset(buffer, 0, MAX_LEN);
        if ((n = recvfrom(sockfd, buffer, MAX_LEN, 0,
                        (struct sockaddr *) &from,  &fromlen)) >= 0) {
            if (!strncmp(buffer, CLOSE_STR, strlen(CLOSE_STR))) {
                continue;
                if (close_sock(sockfd) < 0)
                    break;
            }
            if (!strncmp(buffer, ECHO_STR, strlen(ECHO_STR))) {
                continue;
                if (echo(strstr(buffer, "\n") + 1, sockfd) < 0)
                    break;
            }
            if (!strncmp(buffer, UPLOAD_STR, strlen(UPLOAD_STR))) {
                if(upload(sockfd) < 0)
                    break;;
            }
            if (!strncmp(buffer, TIME_STR, strlen(TIME_STR))) {
                continue;
                if (send_time(sockfd) < 0 )
                    break;
            }
        } else {
            printf("Error recvfrom\n");
        }
    }
    close_sock(sockfd);
    clear();
    return 0;
}
