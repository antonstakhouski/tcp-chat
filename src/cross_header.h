#ifndef H_CROSS_HEADER
#define H_CROSS_HEADER

#define PLATFORM_WIN 1
#define PLATFORM_UNIX 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <inttypes.h>

// this maybe unix only
#include <sys/param.h>
#if defined(_WIN32) || defined(_WIN64)

#include <winsock2.h>
#include <mstcpip.h>
#define PLATFORM PLATFORM_WIN
typedef unsigned int socketlen;

#else

#include <netinet/tcp.h>
#include <stropts.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <aio.h>
#define PLATFORM PLATFORM_UNIX

#define h_addr h_addr_list[0] /* for backward compatibility */
typedef socklen_t socketlen;

#define _POSIX_C_SOURCE 1
int fileno(FILE *stream);

struct aiocb aiocb_;

#endif //  defined(_WIN32) || defined(_WIN64)

enum mode {TCP, UDP};

int close_sock(int sockfd);
void init();
void clear();
void set_keepalive(int sockfd);
void error(const char *msg);
void init_socket(int argc, char* argv[],
        int* sockfd, struct sockaddr_in* serv_addr, enum mode* current_mode);
void print_trans_results(long bytes_sent, time_t trans_time);
void write_file(FILE* file, int nbytes, int offset, char* buffer);
void swap_buffers(char** send_buff, char** read_buff);

#define TCP_MAX_LEN 1432
#define HEADER_LEN 5
#define UDP_MAX_LEN 65000
//#define UDP_MAX_LEN 4096
#define BUFF_ELEMENTS 32
#define BUFFER_LEN UDP_MAX_LEN * BUFF_ELEMENTS
#define CLOSE_STR "CLOSE\n"
#define TIME_STR "TIME\n"
#define ECHO_STR "ECHO\n"
#define UPLOAD_STR "UPLOAD\n"

#define MIN(a,b) (((a)<(b))?(a):(b))

#endif //H_CROSS_HEADER
