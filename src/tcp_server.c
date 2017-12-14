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
