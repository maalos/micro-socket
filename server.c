#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>

void error(char *msg) {
    perror(msg);
    exit(1);
}

// function for setting the http header
void setHttpHeader(char httpHeader[], int code, char *uri) {
    // uri "/" -> "/index.html"
    if (strlen(uri) <= 1)
        uri = "/index.html";
    
    // uri "/***" -> "./***"
    char filepath[100] = ".";
    strcat(filepath, uri);

    // try to open the file
    FILE *htmlData = fopen(filepath, "r");
    if (!htmlData) {
        htmlData = fopen("404.html", "r");
    }

    char *headertext;
    char *headercode;
    char line[100];
    char responseData[8000];

    // clear our buffer so we don't send the same stuff over and over again
    memset(responseData, 0, sizeof(responseData));

    while (fgets(line, 100, htmlData) != 0) {
        strcat(responseData, line);
    }
    
    fclose(htmlData);

    if (code == 404) {
        headercode = "404 Not Found";
    } else if (code == 405) {
        headercode = "405 Method Not Allowed";
    } else {
        headercode = "200 OK";
    }

    headertext = "HTTP/1.1 200 OK\r\n\n";
    //strcat(headertext, headercode);

    strcat(httpHeader, headertext);
    strcat(httpHeader, responseData);
}

int main(int argc, char *argv[]) {
    // socket file descriptor, new socket fd, port number to accept connections on, client address size, return value for write(), read()
    int sockfd, newsockfd, portno, clilen, n;

    // buffer for characters from the socket connection
    char buffer[256];

    // backlog queue size, should not exceed 5
    int bcklog_q = 5;

    // structure containing the internet address
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        *argv[1] = 80;
        // error("No port provided, exiting...");
    }

    // create a socket on the socket file descriptor variable
    // AF_INET - socket address domain, SOCK_STREAM - socket type (stream/datagram), 0 - protocol (0 makes the OS choose between TCP and UDP)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Unable to open socket, exiting...");

    // zero-out the buffer, first arg is a pointer to a buffer, second arg is it's size
    bzero((char *) &serv_addr, sizeof(serv_addr));

    // set the port number to the first argument as int
    portno = atoi(argv[1]);

    // set the structure's fields
    serv_addr.sin_family      = AF_INET; // should always be set to sym const AF_INET
    serv_addr.sin_port        = htons(portno); // port number in host byte -> port number in network byte order
    serv_addr.sin_addr.s_addr = INADDR_ANY; // ip address of the host

    // binding the socket to address
    // arg1 - socket file descriptor, arg2 - bounded-to address, arg3 - size of bounded-to address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("Failed to bind the socket to address, exiting...");
    
    // listen for connections
    // arg1 - socket file descriptor, arg2 - size of the backlog queue, should not exceed 5
    n = listen(sockfd, bcklog_q);
    if (n < 0)
        error("Unable to start listening, exiting...");
    
    int clisock;

    // let's serve forever
    while(1) {
        char httpHeader[8000] = "";

        // structure size
        clilen = sizeof(cli_addr);

        // arg1 - socket file descriptor, arg2 - ref pointer to the address of the client on the other end of the connection, arg3 - structure size
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("Unable to accept connection, exiting...");
        // below code will only execute after a connection is established
        // initialize and then zero-out the buffer for chars from sock conn
        bzero(buffer, 256);
        // read from the new file descriptor to the buffer
        n = read(newsockfd, buffer, 255);
        if (n < 0)
            error("Unable to read from socket, exiting...");
        
        // not needed anymore i guess
        // printf("\n%s\n", buffer);

        // parsing requests into method, uri and protocol
        char *method = strtok(buffer, " \t\r\n");
        char *uri    = strtok(NULL,   " \t");
        char *prot   = strtok(NULL,   " \t\r\n");

        fprintf(stderr, "%s %s %s\n", method, uri, prot);

        setHttpHeader(httpHeader, 200, uri);

        // send the response to client
        n = send(newsockfd, httpHeader, sizeof(httpHeader), 0);
        if (n < 0)
            error("Unable to send to socket, exiting...");

        close(newsockfd);
    }
    return 0;
}
