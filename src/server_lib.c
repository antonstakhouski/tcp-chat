#include "server_lib.h"

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

int udp_upload(int newsockfd)
{
    char rx_buffer1[BUFF_ELEMENTS * (UDP_MAX_LEN - HEADER_LEN)] = {0};
    char rx_buffer2[BUFF_ELEMENTS * (UDP_MAX_LEN - HEADER_LEN)] = {0};
    char *recv_buff = rx_buffer1;
    char *write_buff = rx_buffer2;

    char buffer[UDP_MAX_LEN] = {0};
    char tx_buffer[HEADER_LEN] = {0};

    int n;
    char filename[256] = {0};
    char* size_pos;
    FILE* out_file;
    long filesize;
    size_t filename_len;
    int bytes_received = 0;
    socketlen fromlen = sizeof(struct sockaddr_in);
    struct sockaddr_in from;


    // if we get no info after command
    if ((n = recvfrom(newsockfd, buffer, UDP_MAX_LEN, 0,
                    (struct sockaddr*)&from, &fromlen)) < 0) {
        printf("Error: %s", strerror(n));
        return -1;
    }
    size_pos = strstr(buffer, "\n");
    filename_len = size_pos - buffer;
    strncat(filename, buffer, filename_len);
    printf("Filename: %s\n", filename);
    out_file = fopen(filename, "wb");
    size_pos++;

    filesize= *((long*)size_pos);
    printf("Filesize: %ld\n", filesize);
    printf("%d bytes received\n", n);
    size_t predata = size_pos + sizeof(filesize) - buffer;
    filesize -= fwrite(size_pos + sizeof(filesize), 1, n - predata, out_file);

    int sn = -1;
    int an = 0;
    int ack_size;
    int bytes_to_write;
    unsigned char tx_flags = 0;
    unsigned char rx_flags = 0;

    time_t start_transfer = time(NULL);
    time_t trans_time;
    int buff_elements = 0;
    int first_an = 0;

    int sn_array[BUFF_ELEMENTS] = {-1};
    fd_set readfds;
    struct timeval timeleft;
    long sec = 0;
    long nsec = 500000;
    timeleft.tv_sec = sec;
    timeleft.tv_usec = nsec;
    int sel_res;
    int lost = 0;
    // read from socket to file
    while (1) {
        // read incloming packets
        if (!lost) {
            memset(recv_buff, 0, BUFF_ELEMENTS * (UDP_MAX_LEN - HEADER_LEN));
            first_an =  an;
            buff_elements = 0;
            memset(sn_array, -1, BUFF_ELEMENTS * sizeof(sn_array[0]));
        } else{
            buff_elements = an - first_an;
        }

        while(buff_elements < BUFF_ELEMENTS) {
            FD_SET(newsockfd, &readfds);
            timeleft.tv_sec = sec;
            timeleft.tv_usec = nsec;
            if ((sel_res = select( newsockfd + 1, &readfds, NULL, NULL, &timeleft)) < 0) {
                printf("Error: %s in line %d\n", strerror(n), __LINE__);
            } else if (!sel_res) {
                printf("Timeout occurred!\n");
                lost = 1;
                break;
            } else {
                if (FD_ISSET(newsockfd, &readfds)){ 
                    FD_CLR(newsockfd, &readfds);
                    memset(buffer, 0, UDP_MAX_LEN);
                    if ((n = recvfrom(newsockfd, buffer, UDP_MAX_LEN, 0, (struct sockaddr*)&from, &fromlen)) < 0) {
                        printf("Error: %s", strerror(n));
                        return -1;
                    } else if (!n) {
                        break;
                    }
                    sn = *((int*)buffer);
                    if (sn < first_an)
                        break;
                    rx_flags = *((unsigned char*)(buffer + sizeof(sn)));
                    /*printf("SN: %d\n", sn);*/

                    // terminate connection

                    if (tx_flags & 1) {
                        break;
                    }

                    memcpy(recv_buff + (sn - first_an) * (UDP_MAX_LEN - HEADER_LEN),
                            buffer + HEADER_LEN, UDP_MAX_LEN - HEADER_LEN);
                    sn_array[sn - first_an] = sn;
                    buff_elements++;

                    if (rx_flags & 1) {
                        /*puts("FIN received");*/
                        break;
                    }
                }
            }

        }

        // sort sn array
        /*printf("sorting\n");*/
        int valid;
        int temp_an = -1;
        for (valid = 0; valid < BUFF_ELEMENTS; valid++) {
            /*printf("%d ", sn_array[valid]);*/
            if (sn_array[valid] > -1) {
                temp_an = sn_array[valid];
                break;
            }
        }
        if (temp_an > first_an)
            temp_an = -1;
        if (temp_an > -1) {
            for (int i = valid; i < BUFF_ELEMENTS - 1; i++) {
                if (temp_an + 1 == sn_array[i + 1]) {
                    temp_an++;
                } else {
                    lost = 1;
                }
                /*printf("%d ", sn_array[i + 1]);*/
            }
            an = temp_an + 1;
        }
        

        if (an == sn_array[BUFF_ELEMENTS - 1] + 1) {
            lost = 0;
        }
        /*printf("\n");*/
        

        // write data to file
        if ((!(an - buff_elements - first_an) && !lost ) || (rx_flags & 1)) {
            /*printf("All packets received\n");*/
            if (filesize > 0) {
                bytes_to_write = MIN((UDP_MAX_LEN - HEADER_LEN) * buff_elements, filesize);
                swap_buffers(&write_buff, &recv_buff);
                filesize -= fwrite(write_buff, 1, bytes_to_write, out_file);
                /*write_file(out_file, bytes_to_write, bytes_received, write_buff);*/
                bytes_received += bytes_to_write;
                /*filesize -= bytes_to_write;*/
            }
        } 

        if (rx_flags & 1) {
            tx_flags |= 1;
            /*puts("FIN sent");*/
            memcpy(tx_buffer, &an, sizeof(an));
            memcpy(tx_buffer + sizeof(an), &tx_flags, sizeof(tx_flags));
            ack_size  = sendto(newsockfd, tx_buffer, HEADER_LEN, 0, (struct sockaddr *)&from, fromlen);
            if (ack_size < 0) error("recvfrom");
            sn = -1;
            first_an = an;
            break;
        }

        if ((tx_flags & 1) && !(rx_flags & 1)) {
            goto END_SERVER_UDP_TRANSFER;
        }
        
        // send ACK
        /*printf("AN: %d\n", an);*/
        memcpy(tx_buffer, &an, sizeof(an));
        memcpy(tx_buffer + sizeof(an), &tx_flags, sizeof(tx_flags));
        ack_size  = sendto(newsockfd, tx_buffer, HEADER_LEN, 0, (struct sockaddr *)&from, fromlen);
        if (ack_size < 0) error("recvfrom");

    }
END_SERVER_UDP_TRANSFER:
    close_file(out_file);
    puts("File received");
    trans_time = time(NULL) - start_transfer;
    print_trans_results(bytes_received, trans_time);
    fflush(stdout);
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
    // TODO add lost packets statistics
    time_t trans_time = time(NULL) - start_transfer;
    puts("File received");
    print_trans_results(bytes_received, trans_time);
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
    char buffer[UDP_MAX_LEN];
    struct sockaddr_in from;
    int n;
    socketlen fromlen;
    fromlen = sizeof(struct sockaddr_in);

    puts("Server is ready");
    while (1) {
        memset(buffer, 0, UDP_MAX_LEN);
        if ((n = recvfrom(sockfd, buffer, UDP_MAX_LEN, 0,
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
