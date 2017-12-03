#ifndef H_SERVER_LIB
#define H_SERVER_LIB

#include "cross_header.h"

int echo(char* buff, int newsockfd);
int udp_upload(int newsockfd);
int tcp_upload(int newsockfd);
int send_time(int newsockfd);
void udp_loop(int sockfd);
void tcp_loop(int sockfd);

#endif //H_SERVER_LIB
