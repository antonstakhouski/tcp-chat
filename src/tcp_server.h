#ifndef H_TCP_SERVER
#define H_TCP_SERVER

#include "cross_header.h"
#include <pthread.h>

#define MAX_OPEN_SOCKS 30

struct client_entry {
    int sockfd;
    FILE* file;
    int64_t filesize;
    int64_t bytes_received;
    time_t start_transfer;
};

struct thread_info {
    pthread_t thread_id;
    int thread_num;
    int master_socket;
    char* argv_string;
};

int echo(char* buff, int newsockfd);
int send_time(int newsockfd);
void tcp_loop(int sockfd);
void process_requests(struct client_entry* client, fd_set* readfds, fd_set* errorfds);
static void* thread_start(void* arg);
void tcp_io_loop(int sockfd);

#endif //H_TCP_SERVER
