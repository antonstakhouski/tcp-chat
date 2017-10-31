#include "cross_header.h"

int close_sock(int sockfd)
{
    return closesocket(sockfd);
}
