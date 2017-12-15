#include "tcp_server.h"

//TODO
//fix Windows build
//fix filename bug
//test write to one file

int echo(char* buff, int newsockfd)
{
    char buffer[TCP_MAX_LEN] = {0};

    if (strlen(buff))
        puts(buff);

    if (recv(newsockfd, buffer, TCP_MAX_LEN - 1, 0) < 0)
        return -1;
    if (send(newsockfd, buffer, strlen(buffer), 0) < 0)
        return -1;
    puts(buffer);
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

int tcp_upload(int newsockfd)
{
    char buffer[TCP_MAX_LEN] = {0};
    int n;
    char filename[256] = {0};
    char* size_pos;
    FILE* out_file;
    int64_t filesize;
    size_t filename_len;
    long bytes_received = 0;

    // if we get no info after command
    time_t start_transfer = time(NULL);
    if ((n = recv(newsockfd, buffer, TCP_MAX_LEN, 0)) < 0) {
        printf("Error: %s in line %d\n", strerror(n), __LINE__);
        return -1;
    }
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
    bytes_received += n - predata;

    fd_set set, set_error;
    struct timeval timeleft;
    long sec = 30;
    long nsec = 0;
    timeleft.tv_sec = sec;
    timeleft.tv_usec = nsec;

    int sel_res;
    // read from socket to file
    while (filesize > 0) {
        FD_SET(newsockfd, &set);
        FD_SET(newsockfd, &set_error);
        if ((sel_res = select(newsockfd + 1, &set, NULL, &set_error, &timeleft)) < 0) {
            printf("Error: %s in line %d\n", strerror(n), __LINE__);
        } else if (!sel_res) {
            printf("Timeout occurred!\n");
            break;
        } else {
            if (FD_ISSET(newsockfd, &set_error)) {
                memset(buffer, 0, TCP_MAX_LEN);
                if ((n = recv(newsockfd, buffer, TCP_MAX_LEN, MSG_OOB)) < 0) {
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

            memset(buffer, 0, TCP_MAX_LEN);
            if ((n = recv(newsockfd, buffer, TCP_MAX_LEN, 0)) < 0) {
                printf("Error: %s in line %d\n", strerror(n), __LINE__);
                return -1;
            }
            /*printf("Packet length: %d\n", n);*/
            bytes_received += n;
            filesize -= fwrite(buffer, 1, n, out_file);
        }
    }
    fclose(out_file);
    time_t trans_time = time(NULL) - start_transfer;
    puts("File received");
    print_trans_results(bytes_received, trans_time);
    return 0;
}


void tcp_io_loop(int sockfd)
{
    char buffer[TCP_MAX_LEN];
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
            memset(buffer, 0, TCP_MAX_LEN);
            if ((n = recv(newsockfd, buffer, TCP_MAX_LEN, 0)) > 0) {
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

void tcp_loop(int master_socket)
{
    //listen
    listen(master_socket, 5);

    puts("Server is ready");

    volatile int thread_count = 0;

    int Nmin = 3;
    int Nmax = 10;

    int create_res = 0;
    struct thread_info*  tinfo;
    tinfo = calloc(Nmax, sizeof(struct thread_info));

    for (int i = 0; i < Nmax; i++) {
        tinfo[i].master_socket = master_socket;
    }

    for (int i = 0; i < Nmin; i++) {
        tinfo[i].thread_num = i;

        create_res = pthread_create(&tinfo[i].thread_id, NULL, &thread_start, &tinfo[i]);
        if (create_res != 0) {
            printf("create error\n");
        }
    }

    while(1);
    close_sock(master_socket);
}

static void* thread_start(void* arg)
{
    struct thread_info *tinfo= arg;

    int new_socket;
    struct sockaddr_in cli_addr;
    socketlen clilen;
    clilen = sizeof(cli_addr);

    if ((new_socket = accept(tinfo->master_socket, (struct sockaddr *)&cli_addr, &clilen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    //inform user of socket number - used in send and receive commands
    printf("New connection , socket fd is %d , ip is : %s , port : %d \n" ,
            new_socket , inet_ntoa(cli_addr.sin_addr) , ntohs(cli_addr.sin_port));

    tcp_io_loop(new_socket);
    return NULL;
}
