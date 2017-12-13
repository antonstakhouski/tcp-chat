#include "server_lib.h"

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

int receive_udp_packet(int newsockfd, struct udp_info* udp_info_)
{
    int n;
    int sn;
    char buffer[UDP_MAX_LEN] = {0};
    socketlen fromlen = sizeof(struct sockaddr_in);
    struct sockaddr_in from;

    memset(buffer, 0, UDP_MAX_LEN);
    if ((n = recvfrom(newsockfd, buffer, UDP_MAX_LEN, 0, (struct sockaddr*)&from, &fromlen)) < 0) {
        printf("Error: %s", strerror(n));
        return -1;
    } else if (!n) {
        return 0;
    }
    sn = *((int*)buffer);
    if (sn < udp_info_->first_an)
        return 0;
    udp_info_->rx_flags = *((unsigned char*)(buffer + sizeof(sn)));
    /*printf("SN: %d\n", sn);*/

    // terminate connection

    if (udp_info_->tx_flags & 1) {
        puts("tx_set");
        return 1;
    }

    memcpy(udp_info_->rx_buffer + (sn - udp_info_->first_an) * (UDP_MAX_LEN - HEADER_LEN),
            buffer + HEADER_LEN, UDP_MAX_LEN - HEADER_LEN);
    udp_info_->sn_array[sn - udp_info_->first_an] = sn;
    udp_info_->buff_elements++;

    if ((udp_info_->rx_flags) & 1) {
        puts("FIN received");
        return 1;
    }

    return 0;
}

int receive_udp_chunk(int newsockfd, struct udp_info* udp_info_)
{
    fd_set readfds;
    struct timeval timeleft;
    long sec = 0;
    long nsec = 300000;
    timeleft.tv_sec = sec;
    timeleft.tv_usec = nsec;
    int sel_res;

    // read incloming packets
    if (!(udp_info_->lost)) {
        memset(udp_info_->rx_buffer, 0, BUFF_ELEMENTS * (UDP_MAX_LEN - HEADER_LEN));
        udp_info_->first_an = udp_info_->an;
        udp_info_->buff_elements = 0;
        memset(udp_info_->sn_array, -1, BUFF_ELEMENTS * sizeof(udp_info_->sn_array[0]));
    } else{
        udp_info_->buff_elements = udp_info_->an - udp_info_->first_an;
    }

    while (udp_info_->buff_elements < BUFF_ELEMENTS) {
        FD_SET(newsockfd, &readfds);
        timeleft.tv_sec = sec;
        timeleft.tv_usec = nsec;
        if ((sel_res = select( newsockfd + 1, &readfds, NULL, NULL, &timeleft)) < 0) {
            printf("Error: %s in line %d\n", strerror(sel_res), __LINE__);
        } else if (!sel_res) {
            printf("Timeout occurred!\n");
            udp_info_->lost = 1;
            break;
        } else {
            if (FD_ISSET(newsockfd, &readfds)){
                FD_CLR(newsockfd, &readfds);
                if(receive_udp_packet(newsockfd, udp_info_))
                    return 1;
            }
        }
    }
    return 0;
}

int set_an(struct udp_info* udp_info_)
{
    int lost;
    int valid;
    int temp_an = -1;
    for (valid = 0; valid < BUFF_ELEMENTS; valid++) {
        /*printf("%d ", sn_array[valid]);*/
        if (udp_info_->sn_array[valid] > -1) {
            temp_an = udp_info_->sn_array[valid];
            break;
        }
    }
    if (temp_an > udp_info_->first_an)
        temp_an = -1;
    if (temp_an > -1) {
        for (int i = valid; i < BUFF_ELEMENTS - 1; i++) {
            if (temp_an + 1 == udp_info_->sn_array[i + 1]) {
                temp_an++;
            } else {
                lost = 1;
            }
            /*printf("%d ", sn_array[i + 1]);*/
        }
        udp_info_->an = temp_an + 1;
    }


    if (udp_info_->an == udp_info_->sn_array[BUFF_ELEMENTS - 1] + 1) {
        lost = 0;
    }
    /*printf("\n");*/

    return lost;
}

int write_to_file_udp(struct udp_info* udp_info_, FILE* out_file)
{
    int64_t bytes_to_write;
    // write data to file
    if ((!(udp_info_->an - udp_info_->buff_elements - udp_info_->first_an) && !(udp_info_->lost ))
        || (udp_info_->rx_flags & 1)) {
        /*printf("All packets received\n");*/
        if (udp_info_->filesize > 0) {
            bytes_to_write = MIN((int64_t)(UDP_MAX_LEN - HEADER_LEN) * udp_info_->buff_elements,
                    udp_info_->filesize);
            udp_info_->filesize -= fwrite(udp_info_->rx_buffer, 1, bytes_to_write, out_file);
            udp_info_->bytes_received += bytes_to_write;
        }
    }
    return 0;
}

int udp_transfer_loop(int newsockfd, struct udp_info* udp_info_, FILE* out_file, struct sockaddr_in* from)
{
    int ack_size;
    socketlen fromlen = sizeof(struct sockaddr_in);
    while (1) {
        receive_udp_chunk(newsockfd, udp_info_);
        set_an(udp_info_);
        write_to_file_udp(udp_info_, out_file);

        if (udp_info_->rx_flags & 1) {
            udp_info_->tx_flags |= 1;
            udp_info_->first_an = udp_info_->an;
        }

        if ((udp_info_->tx_flags & 1) && !(udp_info_->rx_flags & 1)) {
            puts("break");
            break;
        }

        // send ACK
        /*printf("AN: %d\n", an);*/
        memcpy(udp_info_->tx_buffer, &(udp_info_->an), sizeof(udp_info_->an));
        memcpy(udp_info_->tx_buffer + sizeof(udp_info_->an), &(udp_info_->tx_flags), sizeof(udp_info_->tx_flags));
        ack_size  = sendto(newsockfd, udp_info_->tx_buffer, HEADER_LEN, 0, (struct sockaddr *)from, fromlen);
        if (ack_size < 0) error("sendto");

    }
    return 0;
}

int udp_upload(int newsockfd)
{
    struct udp_info udp_info_ = {0};
    char buffer[UDP_MAX_LEN] = {0};
    int n;
    char filename[256] = {0};
    char* size_pos;
    FILE* out_file;
    size_t filename_len;
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

    udp_info_.filesize= *((int64_t*)size_pos);
    printf("Filesize: %" PRId64 "\n", udp_info_.filesize);
    printf("%d bytes received\n", n);
    size_t predata = size_pos + sizeof(udp_info_.filesize) - buffer;
    udp_info_.filesize -= fwrite(size_pos + sizeof(udp_info_.filesize), 1, n - predata, out_file);

    time_t start_transfer = time(NULL);
    time_t trans_time;

    memset(udp_info_.sn_array, -1, BUFF_ELEMENTS * sizeof(udp_info_.sn_array[0]));
    udp_info_.lost = 0;

    udp_transfer_loop(newsockfd, &udp_info_, out_file, &from);

    // read from socket to file
    fclose(out_file);
    puts("File received");
    trans_time = time(NULL) - start_transfer;
    print_trans_results(udp_info_.bytes_received, trans_time);
    fflush(stdout);
    return 0;
}

int receive_header(struct client_entry* client)
{
    char buffer[TCP_MAX_LEN] = {0};
    int n;
    char filename[256] = {0};
    char* size_pos;
    size_t filename_len;

    client->start_transfer = time(NULL);
    if ((n = recv(client->sockfd, buffer, TCP_MAX_LEN, 0)) < 0) {
        printf("Error: %s in line %d\n", strerror(n), __LINE__);
        return -1;
    }

    size_pos = strstr(buffer, "\n");
    filename_len = size_pos - buffer;
    printf("Filename len: %zu\n", filename_len);
    strncat(filename, buffer, filename_len);
    printf("Filename: %s\n", filename);
    size_pos++;

    client->file = fopen(filename, "wb");

    client->filesize = *((long*)size_pos);
    printf("Filesize: %" PRId64 "\n", client->filesize);
    size_t predata = size_pos + sizeof(client->filesize) - buffer;
    client->filesize -= fwrite(size_pos + sizeof(client->filesize), 1, n - predata, client->file);
    client->bytes_received = n - predata;
    return 0;
}

int receive_file_chunk(struct client_entry* client, fd_set* readfds, fd_set* errorfds)
{
    char buffer[TCP_MAX_LEN] = {0};

    int n;
    // read from socket to file
    if (client->filesize > 0) {
        if (FD_ISSET(client->sockfd, errorfds)) {
            memset(buffer, 0, TCP_MAX_LEN);
            if ((n = recv(client->sockfd, buffer, TCP_MAX_LEN, MSG_OOB)) < 0) {
                printf("Error: %s in line %d\n", strerror(n), __LINE__);
                return -1;
            }
            else{
                printf("Socket %d file downloaded: %d\n", client->sockfd, *((char*)buffer));
            }
        }

        if (FD_ISSET(client->sockfd, readfds)) {
            memset(buffer, 0, TCP_MAX_LEN);
            if ((n = recv(client->sockfd, buffer, TCP_MAX_LEN, 0)) < 0) {
                printf("Error: %s in line %d\n", strerror(n), __LINE__);
                return -1;
            }
            client->bytes_received += n;
            client->filesize -= fwrite(buffer, 1, n, client->file);
            if(!client->filesize) {
                close_file(client);
            }
        } else {
            printf("Transfer error\n");
            remove_client(client);
            fclose(client->file);
            return -1;
        }
    } else {
        close_file(client);
    }
    return 0;
}

void close_file(struct client_entry* client)
{
    fclose(client->file);
    time_t trans_time = time(NULL) - client->start_transfer;
    puts("File received");
    print_trans_results(client->bytes_received, trans_time);
    client->bytes_received = 0;
    client->file = NULL;
    client->start_transfer = 0;
    client->filesize = 0;
}

int tcp_upload(struct client_entry* client, fd_set* readfds, fd_set* errorfds)
{
    if (!client->file) {
        if (receive_header(client)) {
            return -1;
        }
    } else {
        receive_file_chunk(client, readfds, errorfds);
    }

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

void fill_sets(struct client_entry* clients, fd_set* readfds, fd_set* errorfds, int* max_sd)
{
    //add child sockets to set
    for (int i = 0 ; i < MAX_OPEN_SOCKS; i++)
    {
        //socket descriptor
        int sd = clients[i].sockfd;

        //if valid socket descriptor then add to read list
        if(sd > 0) {
            FD_SET(sd , readfds);
            FD_SET(sd , errorfds);
        }

        //highest file descriptor number, need it for the select function
        if(sd > *max_sd)
            *max_sd = sd;
    }
}

void process_requests(struct client_entry* clients, fd_set* readfds, fd_set* errorfds)
{
    int n;
    char buffer[TCP_MAX_LEN];
    for (int i = 0; i < MAX_OPEN_SOCKS; i++)
    {
        if (FD_ISSET(clients[i].sockfd, readfds) || FD_ISSET(clients[i].sockfd, errorfds))
        {
            // if client is sending file
            if (clients[i].file) {
                if(receive_file_chunk(&clients[i], readfds, errorfds)) {
                    remove_client(&clients[i]);
                }
                break;
            }

            memset(buffer, 0, TCP_MAX_LEN);
            if ((n = recv(clients[i].sockfd, buffer, TCP_MAX_LEN, 0)) > 0) {
                if (!strncmp(buffer, CLOSE_STR, strlen(CLOSE_STR))) {
                    remove_client(&clients[i]);
                }
                if (!strncmp(buffer, ECHO_STR, strlen(ECHO_STR))) {
                    if (echo(strstr(buffer, "\n") + 1, clients[i].sockfd) < 0) {
                        remove_client(&clients[i]);
                    }
                }
                if (!strncmp(buffer, UPLOAD_STR, strlen(UPLOAD_STR))) {
                    if(tcp_upload(&clients[i], readfds, errorfds) < 0) {
                        remove_client(&clients[i]);
                    }
                }
                if (!strncmp(buffer, TIME_STR, strlen(TIME_STR))) {
                    if (send_time(clients[i].sockfd) < 0 ) {
                        remove_client(&clients[i]);
                    }
                }
            }

        }
    }
}

void remove_client(struct client_entry* client) {
    close_sock(client->sockfd);
    client->sockfd = 0;
    if (client->file)
        fclose(client->file);
    client->file = NULL;
    client->filesize = 0;
    client->start_transfer = 0;
    client->bytes_received = 0;
}

void tcp_loop(int master_socket)
{
    //listen
    listen(master_socket, 5);
    int max_sd;
    struct client_entry clients[MAX_OPEN_SOCKS] = {{0}};
    fd_set readfds, errorfds;

    puts("Server is ready");

    while (1) {
        //clear the socket set
        FD_ZERO(&readfds);
        FD_ZERO(&errorfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        fill_sets(clients, &readfds, &errorfds, &max_sd);

        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        int activity = select( max_sd + 1 , &readfds , NULL, &errorfds, NULL);

        if ((activity < 0) && (errno!=EINTR))
        {
            printf("select error");
        }

        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master_socket, &readfds))
            receive_connection(master_socket, clients);

        //else its some IO operation on some other socket :)
        process_requests(clients, &readfds, &errorfds);

    }
    close_sock(master_socket);
}

void receive_connection(int master_socket, struct client_entry* clients)
{
    int new_socket;
    struct sockaddr_in cli_addr;
    socketlen clilen;
    clilen = sizeof(cli_addr);

    if ((new_socket = accept(master_socket, (struct sockaddr *)&cli_addr, &clilen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    //inform user of socket number - used in send and receive commands
    printf("New connection , socket fd is %d , ip is : %s , port : %d \n" ,
            new_socket , inet_ntoa(cli_addr.sin_addr) , ntohs(cli_addr.sin_port));

    //add new socket to array of sockets
    for (int i = 0; i < MAX_OPEN_SOCKS; i++)
    {
        //if position is empty
        if(!clients[i].sockfd)
        {
            clients[i].sockfd = new_socket;
            clients[i].file = NULL;
            clients[i].filesize = 0;
            clients[i].start_transfer = 0;
            clients[i].bytes_received = 0;
            printf("Adding to list of sockets as %d\n" , i);
            break;
        }
    }
}
