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

#endif //H_SERVER_LIB
