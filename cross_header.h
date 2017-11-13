#define PLATFORM_WIN 1
#define PLATFORM_UNIX 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#if defined(_WIN32) || defined(_WIN64)

#include <winsock2.h>
#include <mstcpip.h>
#define PLATFORM PLATFORM_WIN
typedef int socketlen;

#else

#include <netinet/tcp.h>
#include <stropts.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#define PLATFORM PLATFORM_UNIX

typedef socklen_t socketlen;

#endif

int close_sock(int sockfd);
void init();
void clear();
void set_keepalive(int sockfd);

#define MAX_LEN 1432
#define CLOSE_STR "CLOSE\n"
#define TIME_STR "TIME\n"
#define ECHO_STR "ECHO\n"
#define UPLOAD_STR "UPLOAD\n"

#define MIN(a,b) (((a)<(b))?(a):(b))
