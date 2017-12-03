#include "cross_header.h"

void init_socket(int argc, char* argv[],
        int* sockfd, struct sockaddr_in* serv_addr, enum mode* current_mode)
{
    init();

    if (!strcmp(argv[1], "-t")) {
        *current_mode = TCP;
    } else if (!strcmp(argv[1], "-u")) {
        *current_mode = UDP;
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
