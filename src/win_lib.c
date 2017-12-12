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


void set_keepalive(int sockfd)
{
    struct tcp_keepalive alive;
	alive.onoff = 1;
	alive.keepaliveinterval = 2;
	alive.keepalivetime = 30000;
	DWORD ret;

	if (WSAIoctl(sockfd, SIO_KEEPALIVE_VALS, &alive, sizeof(alive), NULL, 0, &ret, NULL, NULL) == SOCKET_ERROR)
	{
    	printf("ERROR KEEPALIVE\n");
	}
}

void write_file(FILE* file, int nbytes, int offset) {
		EnterCriticalSection(&(data->writerSection));
		WaitForSingleObject(data->overRead.hEvent, INFINITE);	
		data->overWrite.Offset = len;
		len += strlen(buf);
		WriteFile(hFile, buf, strlen(buf), NULL,  &(data->overWrite));
}
