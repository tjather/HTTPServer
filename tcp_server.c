/*
 * tcp_server.c - A simple TCP web server
 * usage: ./server <port>
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFSIZE 1024
#define LISTENQ 5 /*maximum number of client connections */

/*
 * getContentType returns format the file contents will be in
 */
char* getContentType(char* filename)
{
    char* contnet = strrchr(filename, '.');
    if (strcmp(contnet, ".html") == 0)
        return "text/html";
    if (strcmp(contnet, ".txt") == 0)
        return "text/plain";
    if (strcmp(contnet, ".png") == 0)
        return "image/png";
    if (strcmp(contnet, ".gif") == 0)
        return "image/gif";
    if (strcmp(contnet, ".jpg") == 0)
        return "image/jpg";
    if (strcmp(contnet, ".ico") == 0)
        return "image/x-icon";
    if (strcmp(contnet, ".css") == 0)
        return "text/css";
    if (strcmp(contnet, ".js") == 0)
        return "application/javascript";
    return "ERROR";
}

/*
 * errorHandler given the appropriate status codes when the HTTP request results in an error
 */
void errorHandler(int client_sock, char version[100], int status_code)
{
    char header[BUFSIZE];

    switch (status_code) {
    case 400:
        printf("400 Bad Request\n");
        snprintf(header, sizeof(header), "%s 400 Bad Request\r\nContent-Length: 0\r\n\r\n", version);
        break;
    case 403:
        printf("403 Forbidden\n");
        printf("%s 404 Bad Request\r\nContent-Length: 0\r\n\r\n", version);

        snprintf(header, sizeof(header), "%s 403 Forbidden\r\nContent-Length: 0\r\n\r\n", version);
        break;
    case 404:
        printf("404 Not Found\n");
        snprintf(header, sizeof(header), "%s 404 Not Found\r\nContent-Length: 0\r\n\r\n", version);
        break;
    case 405:
        printf("405 Method Not Allowed\n");
        snprintf(header, sizeof(header), "%s 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n", version);
        break;
    case 505:
        printf("505 HTTP Version Not Supported\n");
        snprintf(header, sizeof(header), "%s 505 HTTP Version Not Supported\r\nContent-Length: 0\r\n\r\n", version);
        break;
    default:
        status_code = 200;
        printf("200 OK\n");
        snprintf(header, sizeof(header), "%s 200 OK\r\nContent-Length: 0\r\n\r\n", version);
    }

    send(client_sock, header, strlen(header), 0);
}

/*
 * requestHandler processes the request for a single thread that represents one connection
 * it parses the request and transmits the file with all of its contents
 */
void* requestHandler(void* socket_desc)
{
    int client_sock = *(int*)socket_desc;
    int keep_alive;
    struct timeval time;
    int n; /* message byte size */
    char method[100], uri[100], version[100];
    char path[150];
    char header[BUFSIZE];
    int bytes_read;

    free(socket_desc);
    keep_alive = 0;
    time.tv_sec = 10;
    time.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof time);

    do {
        char buffer[BUFSIZE] = { 0 };
        n = recv(client_sock, buffer, BUFSIZE - 1, 0);
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("Timeout occured\n");
                break;
            }
            perror("ERROR in recvfrom");
            break;
        } else if (n == 0) {
            break;
        }

        // printf("Buffer contents: %s", buffer);
        if (strstr(buffer, "Connection: keep-alive") != NULL) {
            keep_alive = 1;
        }

        if (sscanf(buffer, "%s %s %s", method, uri, version) < 3) {
            errorHandler(client_sock, version, 400);
            // close(client_sock);
            // return NULL;
            break;
        }

        if (strcmp(method, "GET") != 0) {
            errorHandler(client_sock, version, 405);
            // close(client_sock);
            // return NULL;
            break;
        }

        if (strcmp(version, "HTTP/1.0") != 0 && strcmp(version, "HTTP/1.1") != 0) {
            errorHandler(client_sock, version, 505);
            // close(client_sock);
            // return NULL;
            break;
        }

        sprintf(path, "./www%s", uri);
        if (path[strlen(path) - 1] == '/') {
            strcat(path, "index.html");
        }

        if (access(path, F_OK) != 0) {
            errorHandler(client_sock, version, 404);
            // close(client_sock);
            // return NULL;
            continue;
        }

        if (access(path, R_OK) != 0) {
            errorHandler(client_sock, version, 403);
            // close(client_sock);
            // return NULL;
            continue;
        }

        FILE* file = fopen(path, "rb");
        if (!file) {
            errorHandler(client_sock, version, 404);
            // close(client_sock);
            // return NULL;
            continue;
        }

        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        char* content_type = getContentType(path);

        if (keep_alive) {
            printf("%s 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: keep-alive\r\n\r\n", version, content_type, file_size);
            snprintf(header, sizeof(header), "%s 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: keep-alive\r\n\r\n", version, content_type, file_size);
        } else {
            printf("%s 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", version, content_type, file_size);
            snprintf(header, sizeof(header), "%s 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", version, content_type, file_size);
        }

        send(client_sock, header, strlen(header), 0);

        char file_buf[BUFSIZE];
        while ((bytes_read = fread(file_buf, 1, BUFSIZE, file)) > 0) {
            // printf("file contents: %s", file_buf);
            send(client_sock, file_buf, bytes_read, 0);
        }

        fclose(file);

        if (!keep_alive) {
            break;
        }
        time.tv_sec = 10;
        time.tv_usec = 0;
        setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof time);

    } while (keep_alive);

    // printf("Closing socket\n");
    close(client_sock);
    return NULL;
}

int main(int argc, char** argv)
{
    int socket_desc; /* socket */
    int portno; /* port to listen on */
    int client_sock; /* connected socket */
    int client_len; /* client size */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    int optval; /* flag value for setsockopt */

    /*
     * check command line arguments
     */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);

    /*
     * socket: create the parent socket
     */
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0)
        perror("ERROR opening socket");
    portno = atoi(argv[1]);

    /* setsockopt: Handy debugging trick that lets
     * us rerun the server immediately after we kill it;
     * otherwise we have to wait about 20 secs.
     * Eliminates "ERROR on binding: Address already in use" error.
     */
    optval = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

    /*
     * build the server's Internet address
     */
    bzero((char*)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    /*
     * bind: associate the parent socket with a port
     */
    if (bind(socket_desc, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        perror("ERROR on binding");

    listen(socket_desc, LISTENQ);

    printf("Server running...waiting for connections.\n");
    client_len = sizeof(struct sockaddr_in);

    while ((client_sock = accept(socket_desc, (struct sockaddr*)&clientaddr, (socklen_t*)&client_len))) {
        if (client_sock < 0) {
            perror("ERROR in accept");
            continue;
        }

        pthread_t thread_id;
        int* new_sock = malloc(sizeof(int));
        if (new_sock == NULL) {
            perror("Failed to allocate memory for socket");
            continue;
        }
        *new_sock = client_sock;

        if (pthread_create(&thread_id, NULL, requestHandler, (void*)new_sock) < 0) {
            perror("ERROR in pthread_create");
            continue;
        }

        pthread_detach(thread_id);
    }

    return 0;
}
