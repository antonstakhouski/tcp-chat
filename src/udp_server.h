#ifndef H_UDP_SERVER
#define H_UDP_SERVER

#include "cross_header.h"

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

int udp_upload(int newsockfd);
void udp_loop(int sockfd);
int receive_udp_packet(int newsockfd, struct udp_info* udp_session_info);
int receive_udp_chunk(int newsockfd, struct udp_info* udp_session_info);
int set_an(struct udp_info* udp_session_info);
int write_to_file_udp(struct udp_info* udp_session_info, FILE* out_file);
int udp_transfer_loop(int newsockfd, struct udp_info* udp_info_, FILE* out_file, struct sockaddr_in* from);

#endif //H_UDP_SERVER
