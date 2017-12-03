#ifndef H_CLIENT_LIB
#define H_CLIENT_LIB

#include "cross_header.h"

void get_cmd(int opcode, char* cmd);
void get_time(int sockfd);
void echo(int sockfd);
void udp_upload(char* filename, int sockfd, const struct sockaddr* server);
void tcp_upload(char* filename, int sockfd);
void show_tcp_help();
void tcp_loop(int sockfd);
void udp_loop(int sockfd, struct sockaddr_in serv_addr);

#endif //H_CLIENT_LIB
