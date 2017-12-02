#include "../cross_header.h"

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void get_cmd(int opcode, char* cmd)
{
    switch(opcode) {
        case 1:
            strcpy(cmd, TIME_STR);
            break;
        case 2:
            strcpy(cmd, ECHO_STR);
            break;
        case 3:
            strcpy(cmd, UPLOAD_STR);
            break;
        case 0:
            strcpy(cmd, CLOSE_STR);
    }
}

void get_time(int sockfd)
{
    char buffer[MAX_LEN] = {0};
    recv(sockfd, buffer, MAX_LEN, 0);
    printf("%s", buffer);
}

void echo(int sockfd)
{
    char buffer[MAX_LEN];
    scanf("%s", buffer);
    send(sockfd, buffer, strlen(buffer), 0);
    recv(sockfd, buffer, MAX_LEN, 0);
    printf("%s\n", buffer);
}

void upload(char* filename, int sockfd, const struct sockaddr* server)
{
    FILE* file;
    char buffer[MAX_LEN];
    int bytes_sent = 0;
    int res = 0;
    int data_sent = 0;
    unsigned int length = sizeof(struct sockaddr_in);

    if(!(file = fopen(filename, "rb"))) {
        puts("error opening file");
        return;
    }
    size_t size = 0;
    memset(buffer, 0, MAX_LEN);
    strcpy(buffer, filename);
    strcat(buffer, "\n");

    fseek(file, 0L, SEEK_END);
    long filesize = ftell(file);
    rewind(file);
    int char_end = strlen(buffer);
    memcpy(buffer + char_end, &filesize, sizeof(filesize));
    printf("Filesize: %ld\n", filesize);

    if ((res = sendto(sockfd, buffer, char_end + sizeof(filesize), 0,
                    server, length)) < 0) {
        printf("Error %s\n", strerror(res));
        return;
    } else {
        bytes_sent += res;
        printf("%d bytes sent\n", bytes_sent);
    }

    memset(buffer, 0, MAX_LEN);
    while (( size = fread(buffer, 1, MAX_LEN, file))) {
        if ((res = sendto(sockfd, buffer, size, 0, server, length)) < 0) {
            printf("Error %s\n", strerror(res));
            return;
        } else {
            bytes_sent += res;
            data_sent += res;
            printf("%ld%%\n", (data_sent * 100) / filesize);
        }
        memset(buffer, 0, MAX_LEN);
    }
    fclose(file);
}

void show_help()
{
    puts("Choose action:");
    puts("1 - TIME");
    puts("2 - ECHO");
    puts("3 - UPLOAD");
    puts("0 - EXIT");
}

int main(int argc, char *argv[])
{
    init();
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    /** struct sockaddr_in form; */
    struct hostent *server;

    if (argc < 3) {
       fprintf(stderr, "usage %s hostname port\n", argv[0]);
       exit(0);
    }

    //create socket
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(sockfd >= 0);

    server = gethostbyname(argv[1]);
    assert(server);

    //bind socket
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr,
            (char *)server->h_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    unsigned int length = sizeof(struct sockaddr_in);

    int choize;
    char cmd[256];
    int n;
    while (1) {
        show_help();

        int res;
        res = scanf("%d", &choize);
        while(!res || choize < 0 || choize > 3)
            res = scanf("%d", &choize);
        get_cmd(choize, cmd);
        n = sendto(sockfd, cmd, strlen(cmd), 0,
                (const struct sockaddr*)&serv_addr, length);
        if (n < 0) error("sendto");

        if (!strcmp(cmd, TIME_STR)) {
            continue;
            get_time(sockfd);
        }
        if (!strcmp(cmd, ECHO_STR)) {
            continue;
            echo(sockfd);
        }
        if (!strcmp(cmd, UPLOAD_STR)) {
            char filename[256];
            puts("Enter filename");
            scanf("%s", filename);
            upload(filename, sockfd, (const struct sockaddr*)&serv_addr);
        }
        if (!strcmp(cmd, CLOSE_STR)) {
            /** continue; */
            /** close_sock(sockfd); */
            clear();
            return 0;
        }
    }

    clear();
    return 0;
}
