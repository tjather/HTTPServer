/*
 * tcp_client.c - A simple UDP client
 * usage: client <host> <port>
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 4096

int main(int argc, char** argv)
{
    int sockfd;
    int port;
    char* hostname;
    char buf[BUFSIZE];
    struct sockaddr_in serveraddr;
    struct hostent* server;

    int n;
    char sendline[BUFSIZE], recvline[BUFSIZE];

    /* check command line arguments */
    if (argc != 3) {
        fprintf(stderr, "usage: %s <hostname> <port>\n", argv[0]);
        exit(0);
    }

    hostname = argv[1];
    port = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char*)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(port);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    // Connection of the client to the socket
    if (connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("Problem in connecting to the server");
        exit(3);
    }

    while (fgets(sendline, BUFSIZE, stdin) != NULL) {

        send(sockfd, sendline, strlen(sendline), 0);

        if (recv(sockfd, recvline, BUFSIZE, 0) == 0) {
            // error: server terminated prematurely
            perror("The server terminated prematurely");
            exit(4);
        }
        printf("%s", "String received from the server: ");
        fputs(recvline, stdout);
    }

    return 0;
}