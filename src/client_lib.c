#include "client_lib.h"

void get_cmd(int opcode, char* cmd)
{
    switch(opcode) {
        case 1:
            strcpy(cmd, TIME_STR);
            break;
        case 2:
            strcpy(cmd, ECHO_STR);
            break;
        case 3:
            strcpy(cmd, UPLOAD_STR);
            break;
        case 0:
            strcpy(cmd, CLOSE_STR);
    }
}

void get_time(int sockfd)
{
    char buffer[MAX_LEN] = {0};
    recv(sockfd, buffer, MAX_LEN, 0);
    printf("%s", buffer);
}

void echo(int sockfd)
{
    char buffer[MAX_LEN];
    scanf("%s", buffer);
    send(sockfd, buffer, strlen(buffer), 0);
    recv(sockfd, buffer, MAX_LEN, 0);
    printf("%s\n", buffer);
}

void udp_upload(char* filename, int sockfd, const struct sockaddr* server)
{
    FILE* file;
    char tx_buffer[MAX_LEN] = {0};
    char rx_buffer[MAX_LEN] = {0};
    unsigned int bytes_sent = 0;
    unsigned int bytes_lost = 0;
    int res = 0;
    unsigned int length = sizeof(struct sockaddr_in);

    if(!(file = fopen(filename, "rb"))) {
        puts("error opening file");
        return;
    }
    res = sendto(sockfd, UPLOAD_STR, strlen(UPLOAD_STR), 0,
            server, length);
    if (res < 0) error("sendto");

    size_t size = 0;
    strcpy(tx_buffer, filename);
    strcat(tx_buffer, "\n");

    fseek(file, 0L, SEEK_END);
    long filesize = ftell(file);
    rewind(file);
    int char_end = strlen(tx_buffer);
    memcpy(tx_buffer + char_end, &filesize, sizeof(filesize));
    printf("Filesize: %ld\n", filesize);

    time_t start_transfer = time(NULL);
    if ((res = sendto(sockfd, tx_buffer, char_end + sizeof(filesize), 0,
                    server, length)) < 0) {
        printf("Error %s\n", strerror(res));
        return;
    } else {
        bytes_sent += res;
        printf("%d bytes sent\n", bytes_sent);
    }

    memset(tx_buffer, 0, MAX_LEN);
    int sn = 0, an = -1;
    memcpy(tx_buffer, &sn, sizeof(sn));
    int ack_size;
    struct sockaddr_in from;

    while (( size = fread(tx_buffer + sizeof(sn), 1, MAX_LEN - sizeof(sn), file))) {
        if ((res = sendto(sockfd, tx_buffer, MAX_LEN, 0, server, length)) < 0) {
            printf("Error %s\n", strerror(res));
            return;
        } else {
            while (an <= sn) {
                memset(rx_buffer, 0, MAX_LEN);

                // receive ACK
                ack_size = recvfrom(sockfd, rx_buffer, MAX_LEN, 0, (struct sockaddr *)&from, &length);
                if (ack_size < 0) error("recvfrom");
                an = *((int*)rx_buffer);
                printf("SN: %d ", sn);
                printf("AN: %d\n", an);

                // ACK was lost. retransmit package
                bytes_sent += res;
                if (an <= sn) {
                    bytes_lost += res;
                    printf("Packet %d lost\n", sn);
                    if ((res = sendto(sockfd, tx_buffer, MAX_LEN, 0, server, length)) < 0) {
                        printf("Error %s\n", strerror(res));
                        return;
                    }
                } else {
                    break;
                }
            }
            sn++;
            an = -1;
            memset(tx_buffer, 0, MAX_LEN);
            memcpy(tx_buffer, &sn, sizeof(sn));
        }
    }
    time_t trans_time = time(NULL) - start_transfer;
    print_trans_results(bytes_sent, trans_time);
    fclose(file);
}

void tcp_upload(char* filename, int sockfd)
{
    FILE* file;
    char buffer[MAX_LEN] = {0};
    unsigned int bytes_sent = 0;
    int res = 0;
    unsigned int data_sent = 0;

    if(!(file = fopen(filename, "rb"))) {
        puts("error opening file");
        return;
    }
    send(sockfd, UPLOAD_STR, MAX_LEN, 0);

    size_t size = 0;
    memset(buffer, 0, MAX_LEN);
    strcpy(buffer, filename);
    strcat(buffer, "\n");

    fseek(file, 0L, SEEK_END);
    int64_t filesize = ftell(file);
    rewind(file);
    int char_end = strlen(buffer);
    memcpy(buffer + char_end, &filesize, sizeof(filesize));
    printf("Filesize: %" PRId64 "\n", filesize);
    printf("Filesize takes %zu bytes\n", sizeof(filesize));

    time_t start_transfer = time(NULL);
    if ((res = send(sockfd, buffer, char_end + sizeof(filesize), 0)) < 0) {
        printf("Error %s\n", strerror(res));
        return;
    } else {
        bytes_sent += res;
        printf("%d bytes sent\n", res);
    }

    memset(buffer, 0, MAX_LEN);

    int oob_counter = 0;
    int oob_interval = 4000;
    while (( size = fread(buffer, 1, MAX_LEN, file))) {
        if ((res = send(sockfd, buffer, size, 0)) < 0) {
            printf("Error %s\n", strerror(res));
            return;
        } else {
            /*printf("%d bytes sent\n", res);*/
            bytes_sent += res;
            data_sent += res;
        }
        memset(buffer, 0, MAX_LEN);

        char procents = ((data_sent * 100) / filesize) % 10;
        if (oob_counter > oob_interval) {
            if ((res = send(sockfd, (char*)&procents, sizeof(procents), MSG_OOB)) < 0) {
                printf("Error %s\n", strerror(res));
                return;
            } else {
                printf("OOB data: %d\n", procents);
            }
            oob_counter = 0;
        }
        else {
            oob_counter++;
        }
    }
    time_t trans_time = time(NULL) - start_transfer;
    print_trans_results(bytes_sent, trans_time);
    fclose(file);
}

void print_trans_results(long bytes_sent, time_t trans_time)
{
    printf("%ld bytes transferred in: %lds\n", bytes_sent, trans_time);
    printf("Transfer speed is: %f Mb/s \n",
            ((float)bytes_sent * 8 / trans_time) / (1000 * 1000));
}

void show_tcp_help()
{
    puts("Choose action:");
    puts("1 - TIME");
    puts("2 - ECHO");
    puts("3 - UPLOAD");
    puts("0 - EXIT");
}

void tcp_loop(int sockfd)
{
    int choize;
    char cmd[256];
    while (1) {
        show_tcp_help();

        int res;
        res = scanf("%d", &choize);
        while(!res || choize < 0 || choize > 3)
            res = scanf("%d", &choize);
        get_cmd(choize, cmd);

        if (!strcmp(cmd, TIME_STR)){
            send(sockfd, cmd, strlen(cmd), 0);
            get_time(sockfd);
        }
        if (!strcmp(cmd, ECHO_STR)){
            send(sockfd, cmd, strlen(cmd), 0);
            echo(sockfd);
        }
        if (!strcmp(cmd, UPLOAD_STR)) {
            char filename[256];
            puts("Enter filename");
            scanf("%s", filename);
            tcp_upload(filename, sockfd);
        }
        if (!strcmp(cmd, CLOSE_STR)) {
            send(sockfd, cmd, strlen(cmd), 0);
            close_sock(sockfd);
            clear();
            return;
        }
    }
}

void udp_loop(int sockfd, struct sockaddr_in serv_addr)
{
    while (1) {
            char filename[256];
            puts("Enter filename");
            scanf("%s", filename);
            udp_upload(filename, sockfd, (const struct sockaddr*)&serv_addr);
    }
}
