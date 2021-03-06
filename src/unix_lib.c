#include "cross_header.h"

int close_sock(int sockfd)
{
    return close(sockfd);
}

void init()
{
    return;
}

void clear()
{
    return;
}

void set_keepalive(int sockfd)
{
    int optval;
    socklen_t optlen = sizeof(optval);

    /* Set the option active */
    optval = 1;
    optlen = sizeof(optval);
    if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        perror("setsockopt()");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    optval = 5;
    if(getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &optval, &optlen) < 0) {
        perror("getsockopt()");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    optval = 30;
    if(getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &optval, &optlen) < 0) {
        perror("getsockopt()");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    optval = 2;
    if(getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &optval, &optlen) < 0) {
        perror("getsockopt()");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}
