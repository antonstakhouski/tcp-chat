#include "cross_header.h"

int close_sock(int sockfd)
{
    return closesocket(sockfd);
}

void init()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);
    return;
}

void clear()
{
    WSACleanup();
}
