#include "cross_header.h"

void init_socket(int argc, char* argv[],
        int* sockfd, struct sockaddr_in* serv_addr, enum mode* current_mode)
{
    init();

    if (!strcmp(argv[1], "-t")) {
        *current_mode = TCP;
        puts("TCP mode");
    } else if (!strcmp(argv[1], "-u")) {
        *current_mode = UDP;
        puts("UDP mode");
    } else {
        puts("You must specify transport layer protocol\n");
        exit(0);
    }

    if (argc < 4) {
       fprintf(stderr, "usage %s mode hostname port\n", argv[0]);
       exit(0);
    }

    //create socket
    int portno;
    portno = atoi(argv[3]);
    if (*current_mode == TCP)
        *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    else
        *sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(*sockfd >= 0);


    //bind socket
    memset((char *) serv_addr, 0, sizeof(*serv_addr));
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons(portno);

    struct hostent *server;
    server = gethostbyname(argv[2]);
    assert(server);
    memcpy((char *)&(serv_addr->sin_addr.s_addr),
            (char *)server->h_addr,
         server->h_length);

    if (*current_mode == TCP)
        set_keepalive(*sockfd);
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void print_trans_results(long bytes_sent, time_t trans_time)
{
    printf("%ld bytes transferred in: %lds\n", bytes_sent, trans_time);
    printf("Transfer speed is: %f Mb/s \n",
            ((float)bytes_sent * 8 / trans_time) / (1000 * 1000));
}

void swap_buffers(char** buff1, char** buff2)
{
    char* p;
    p = *buff1;
    *buff1 = *buff2;
    *buff2 = p;
}
