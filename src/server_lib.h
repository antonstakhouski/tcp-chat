#ifndef H_SERVER_LIB
#define H_SERVER_LIB

#include "cross_header.h"

#define MAX_OPEN_SOCKS 30

struct client_entry {
    int sockfd;
    FILE* file;
    int64_t filesize;
    int64_t bytes_received;
    time_t start_transfer;
};

struct udp_info {
    int an;
    int64_t bytes_received;
    int64_t filesize;
    int buff_elements;
    char rx_buffer[BUFF_ELEMENTS * (UDP_MAX_LEN - HEADER_LEN)];
    char tx_buffer[HEADER_LEN];
    int sn_array[BUFF_ELEMENTS];
    int first_an;
    unsigned char tx_flags;
    unsigned char rx_flags;
    int lost;
};

int echo(char* buff, int newsockfd);
int udp_upload(int newsockfd);
int tcp_upload(struct client_entry* client, fd_set* readfds, fd_set* errorfds);
int send_time(int newsockfd);
void udp_loop(int sockfd);
void tcp_loop(int sockfd);
void receive_connection(int master_socket, struct client_entry* client);
void fill_sets(struct client_entry* client, fd_set* readfds, fd_set* errorfds, int* max_sd);
void process_requests(struct client_entry* client, fd_set* readfds, fd_set* errorfds);
void remove_client(struct client_entry* client);
int receive_header(struct client_entry* client);
int receive_file_chunk(struct client_entry* client, fd_set* readfds, fd_set* errorfds);
void close_file(struct client_entry* client);

int receive_udp_packet(int newsockfd, struct udp_info* udp_session_info);
int receive_udp_chunk(int newsockfd, struct udp_info* udp_session_info);
int set_an(struct udp_info* udp_session_info);
int write_to_file_udp(struct udp_info* udp_session_info, FILE* out_file);
int udp_transfer_loop(int newsockfd, struct udp_info* udp_info_, FILE* out_file, struct sockaddr_in* from);

#endif //H_SERVER_LIB
