#include "csapp.h"
#include <stdio.h>
#include <string.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char* user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char* conn_close_hdr = "Connection: close\r\n";
static const char* proxy_conn_close_hdr = "Proxy-Connection: close\r\n";

void proxy(int fd);
void clienterror(int fd, char* cause, char* errnum,
    char* shortmsg, char* longmsg);
void parse_request(char* buf, char* method, char* hostname,
    char* query, char* version);

int main(int argc, char** argv)
{
    // check command line args
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    // setup a proxy server for listening
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE,
            port, MAXLINE, 0);
        printf("****************************\n");
        printf("Accepted connection from (%s, %s) with fd\n", hostname, port, connfd);
        proxy(connfd);
        printf("****************************\n\n");
        Close(connfd);
    }

    exit(0);
}

void proxy(int clientfd) {
    // handle the input from the client
    char buf[MAXLINE];
    char method[MAXLINE], hostname[MAXLINE], query[MAXLINE], version[MAXLINE];
    rio_t clientio;

    Rio_readinitb(&clientio, clientfd);
    if (!Rio_readlineb(&clientio, buf, MAXLINE))
        return;
    printf("Original request:\n* %s", buf);

    parse_request(buf, method, hostname, query, version);


    // test the parse result
    printf("Request parsing: \n");
    printf("* method: %s\n", method);
    printf("* hostname: %s\n", hostname);
    printf("* query: %s\n", query);
    printf("* version: %s\n", version);

    // TODO: Are the checks needed cause the servers will check?
    // only accepts "GET" request
    if (strcasecmp(method, "GET")) {
        clienterror(clientfd, method, "501", "Not Implemented",
            "Proxy does not implement this method");
        return;
    }

    // check the http version
    if (strcasecmp(version, "HTTP/1.0") && strcasecmp(version, "HTTP/1.1")) {
        clienterror(clientfd, version, "505", "Unsupported HTTP Version",
            "Need to be HTTP/1.1 or HTTP/1.0");
        return;
    }

    int serverfd;
    char* port_index;
    char port[MAXLINE];
    rio_t serverio;

    // get rid of http prefix
    if (strstr(hostname, "http")) {
        strcpy(buf, hostname);
        strcpy(hostname, buf + 7);
    }

    // get port number from the hostname
    port_index = rindex(hostname, ':');
    if (port_index == NULL)
        strcpy(port, "80");
    else {
        strcpy(port, port_index + 1);
        *port_index = '\0';
    }

    printf("Hostname and port number: \n");
    printf("* hostname: %s\n", hostname);
    printf("* port number: %s\n", port);

    // get the request command to send to server
    strcpy(buf, "GET ");
    strcat(buf, query);
    strcat(buf, " HTTP/1.0\r\n");

    // connect to the server to be requested
    serverfd = open_clientfd(hostname, port);
    if (serverfd < 0) {
        clienterror(clientfd, strcat(hostname, port), "400", "Request Error",
            "Can't connect to the server via given hostname and port");
        return;
    }
    Rio_readinitb(&serverio, serverfd);

    // forward the request to the server
    Rio_writen(serverfd, buf, strlen(buf));  // GET
    while (1) {
        Rio_readlineb(&clientio, buf, MAXLINE); // read header from client
        if (!strcmp(buf, "\r\n"))
            break;
        Rio_writen(serverfd, buf, strlen(buf)); // forward header to server
    }

    // add extra headers
    // Rio_writen(clientfd, user_agent_hdr, strlen(user_agent_hdr));
    // Rio_writen(clientfd, conn_close_hdr, strlen(conn_close_hdr));
    // Rio_writen(clientfd, proxy_conn_close_hdr, strlen(proxy_conn_close_hdr));
    Rio_writen(serverfd, buf, strlen(buf));  // empty line for ending

    // struct stat stat;
    // fstat(clientfd, &stat);


    // read response body from server and forward it to client
    size_t n;
    while ((n = Rio_readlineb(&serverio, buf, MAXLINE)) != 0)
    {
        printf("proxy received %d bytes,then send\n", n);
        Rio_writen(clientfd, buf, n);
    }

    Close(serverfd);
}


void parse_request(char* buf, char* method, char* hostname,
    char* query, char* version) {
    char uri[MAXLINE];
    size_t urilen;
    int index;

    sscanf(buf, "%s %s %s", method, uri, version);
    urilen = strlen(uri);
    for (index = 0; index < urilen; ++index) {
        if (uri[index] == '/' &&
            (index == 0 || uri[index - 1] != '/') &&
            (index == urilen - 1 || uri[index + 1] != '/'))
            break;
        else
            hostname[index] = uri[index];
    }
    hostname[index] = '\0';

    strcpy(query, uri + index);
}


void clienterror(int fd, char* cause, char* errnum,
    char* shortmsg, char* longmsg)
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Proxy Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The tiny proxy</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}