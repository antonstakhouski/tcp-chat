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
    char buffer[TCP_MAX_LEN] = {0};
    recv(sockfd, buffer, TCP_MAX_LEN, 0);
    printf("%s", buffer);
}

void echo(int sockfd)
{
    char buffer[TCP_MAX_LEN];
    scanf("%s", buffer);
    send(sockfd, buffer, strlen(buffer), 0);
    recv(sockfd, buffer, TCP_MAX_LEN, 0);
    printf("%s\n", buffer);
}

void swap_buffers(char** send_buff, char** read_buff)
{
    char* p;
    p = *send_buff;
    *send_buff= *read_buff;
    *read_buff= p;
}

void udp_upload(char* filename, int sockfd, const struct sockaddr* server)
{
    FILE* file;
    char tx_buffer1[BUFFER_LEN] = {0};
    char tx_buffer2[BUFFER_LEN] = {0};
    char rx_buffer[HEADER_LEN] = {0};
    char *send_buff;
    char *read_buff;
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

    strcpy(tx_buffer1, filename);
    strcat(tx_buffer1, "\n");

    fseek(file, 0L, SEEK_END);
    long filesize = ftell(file);
    rewind(file);
    int char_end = strlen(tx_buffer1);
    memcpy(tx_buffer1 + char_end, &filesize, sizeof(filesize));
    printf("Filesize: %ld\n", filesize);

    if ((res = sendto(sockfd, tx_buffer1, char_end + sizeof(filesize), 0,
                    server, length)) < 0) {
        printf("Error %s\n", strerror(res));
        return;
    } else {
        printf("%d bytes sent\n", res);
    }

    memset(tx_buffer1, 0, char_end + sizeof(filesize));
    int sn = 0, an = -1;
    unsigned char tx_flags = 0;
    unsigned char rx_flags = 0;
    int ack_size;
    struct sockaddr_in from;

    time_t start_transfer = time(NULL);
    time_t trans_time;
    size_t size = 1;

    int first_sn = 0;
    int lost = 0;

    // double bufferization
    read_buff = tx_buffer1;
    send_buff = tx_buffer2;

    int buff_elements = 0;
    memset(read_buff, 0, BUFFER_LEN);
    while (buff_elements < BUFF_ELEMENTS) {
        // form new packet
        size = fread(read_buff + UDP_MAX_LEN * buff_elements + HEADER_LEN, 1, UDP_MAX_LEN - HEADER_LEN, file);
        // set FIN flag
        if (!size) {
            /*puts("FIN sent");*/
            tx_flags |= 1;
        }
        bytes_sent += size;
        memcpy(read_buff + UDP_MAX_LEN * buff_elements, &sn, sizeof(sn));
        memcpy(read_buff + UDP_MAX_LEN * buff_elements + sizeof(sn), &tx_flags, sizeof(tx_flags));
        /*printf("SN: %d\n", sn);*/
        sn++;
        buff_elements++;
        if (!size)
            break;
    }

    while (1) {
        // form BUFF_ELEMENTS packets
        if (!lost) {
            // transfer all packets
            swap_buffers(&read_buff, &send_buff);
            first_sn = *((int*)send_buff);
            for(int i = 0; i < buff_elements; i++)
            {
                if ((res = sendto(sockfd, send_buff + i * UDP_MAX_LEN, UDP_MAX_LEN, 0, server, length)) < 0) {
                    printf("Error %s\n", strerror(res));
                    return;
                }
            }

            buff_elements = 0;
            memset(read_buff, 0, BUFFER_LEN);
            while (buff_elements < BUFF_ELEMENTS) {
                // form new packet
                size = fread(read_buff + UDP_MAX_LEN * buff_elements + HEADER_LEN, 1, UDP_MAX_LEN - HEADER_LEN, file);
                // set FIN flag
                if (!size) {
                    /*puts("FIN sent");*/
                    tx_flags |= 1;
                }
                bytes_sent += size;
                memcpy(read_buff + UDP_MAX_LEN * buff_elements, &sn, sizeof(sn));
                memcpy(read_buff + UDP_MAX_LEN * buff_elements + sizeof(sn), &tx_flags, sizeof(tx_flags));
                /*printf("SN: %d\n", sn);*/
                sn++;
                buff_elements++;
                if (!size)
                    break;
            }
        }

        // wait ACK
        int new_sn = *((int*)read_buff);
        while (an < new_sn - 1) {
            memset(rx_buffer, 0, HEADER_LEN);

            // receive ACK
            ack_size = recvfrom(sockfd, rx_buffer, HEADER_LEN, 0, (struct sockaddr *)&from, &length);
            if (ack_size < 0) error("recvfrom");
            an = *((int*)rx_buffer);
            rx_flags = *((unsigned char*)(rx_buffer + sizeof(an)));
            /*printf("SN: %d ", new_sn);*/
            /*printf("AN: %d\n", an);*/

            // SOME packets lost
            if (an < new_sn - 1) {
                bytes_lost += res;
                printf("Packet %d lost\n", an);
                lost = 1;
                for (int i = an; i < new_sn; i++) {
                    if ((res = sendto(sockfd, send_buff + (i - first_sn) * UDP_MAX_LEN, UDP_MAX_LEN,
                                    0, server, length)) < 0) {
                        printf("Error %s\n", strerror(res));
                        return;
                    }
                }
            } else {
                lost = 0;

                if (rx_flags & 1) {
                    /*puts("FIN received");*/
                    memset(send_buff, 0, BUFFER_LEN);
                    sn++;
                    memcpy(send_buff, &sn, sizeof(sn));
                    tx_flags &= ~1;
                    memcpy(send_buff+ sizeof(sn), &tx_flags, sizeof(tx_flags));
                    if ((res = sendto(sockfd, send_buff, HEADER_LEN, 0, server, length)) < 0) {
                        printf("Error %s\n", strerror(res));
                        return;
                    }
                    goto END_CLIENT_UDP_TRANSFER;
                }
                break;
            }
        }
        an = -1;
    }
END_CLIENT_UDP_TRANSFER:
    trans_time = time(NULL) - start_transfer;
    print_trans_results(bytes_sent, trans_time);
    fflush(stdout);
    fclose(file);
}

void tcp_upload(char* filename, int sockfd)
{
    FILE* file;
    char buffer[TCP_MAX_LEN] = {0};
    unsigned int bytes_sent = 0;
    int res = 0;

    if(!(file = fopen(filename, "rb"))) {
        puts("error opening file");
        return;
    }
    send(sockfd, UPLOAD_STR, TCP_MAX_LEN, 0);

    size_t size = 0;
    memset(buffer, 0, TCP_MAX_LEN);
    strcpy(buffer, filename);
    strcat(buffer, "\n");

    fseek(file, 0L, SEEK_END);
    int64_t filesize = ftell(file);
    rewind(file);
    int char_end = strlen(buffer);
    memcpy(buffer + char_end, &filesize, sizeof(filesize));
    printf("Filesize: %" PRId64 "\n", filesize);

    time_t start_transfer = time(NULL);
    if ((res = send(sockfd, buffer, char_end + sizeof(filesize), 0)) < 0) {
        printf("Error %s\n", strerror(res));
        return;
    } else {
        printf("%d bytes sent\n", res);
    }

    memset(buffer, 0, TCP_MAX_LEN);

    int oob_counter = 0;
    int oob_interval = 4000;
    char procents;
    while (( size = fread(buffer, 1, TCP_MAX_LEN, file))) {
        if ((res = send(sockfd, buffer, size, 0)) < 0) {
            printf("Error %s\n", strerror(res));
            return;
        } else {
            /*printf("%d bytes sent\n", res);*/
            bytes_sent += res;
        }
        memset(buffer, 0, TCP_MAX_LEN);

        if (oob_counter > oob_interval) {
            procents = ((int64_t)bytes_sent * 100) / filesize;
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
