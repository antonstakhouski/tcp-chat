#include "cross_header.h"
#include "server_lib.h"

int main(int argc, char *argv[])
{
    enum mode current_mode;
    struct sockaddr_in serv_addr;
    int sockfd;
    init_socket(argc, argv, &sockfd, &serv_addr, &current_mode);

    assert(!bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));
    if (current_mode == TCP)
        tcp_loop(sockfd);
    else
        udp_loop(sockfd);
    close_sock(sockfd);
    clear();
    return 0;
}
