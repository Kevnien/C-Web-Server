/**
 * webserver.c -- A webserver written in C
 * 
 * Test with curl (if you don't have it, install it):
 * 
 *    curl -D - http://localhost:3490/
 *    curl -D - http://localhost:3490/d20
 *    curl -D - http://localhost:3490/date
 * 
 * You can also test the above URLs in your browser! They should work!
 * 
 * Posting Data:
 * 
 *    curl -D - -X POST -H 'Content-Type: text/plain' -d 'Hello, sample data!' http://localhost:3490/save
 * 
 * (Posting data is harder to test from a browser.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/file.h>
#include <fcntl.h>
#include "net.h"
#include "file.h"
#include "mime.h"
#include "cache.h"

#define PORT "3490"  // the port users will be connecting to

#define SERVER_FILES "./serverfiles"
#define SERVER_ROOT "./serverroot"

/**
 * Send an HTTP response
 *
 * header:       "HTTP/1.1 404 NOT FOUND" or "HTTP/1.1 200 OK", etc.
 * content_type: "text/plain", etc.
 * body:         the data to send.
 * 
 * Return the value from the send() function.
 */
int send_response(int fd, char *header, char *content_type, void *body, int content_length)
{
    const int max_response_size = 65536*5;
    char response[max_response_size];

    // Build HTTP response and store it in response

    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
    time_t seconds;
    struct tm *info;

    time(&seconds);
    info = localtime(&seconds);

    int response_length = sprintf(response,
        "%s\n"
        "Date: %s"
        "Connection: close\n"
        "Content-Length %d"
        "Content-Type: %s\n"
        "\n",
        header,
        asctime(info),
        content_length,
        content_type
    );

    memcpy(response+response_length, body, content_length);

    // Send it all!
    int rv = send(fd, response, response_length+content_length, 0);

    if (rv < 0) {
        perror("send");
    }

    return rv;
}


/**
 * Send a /d20 endpoint response
 */
void get_d20(int fd)
{
    // Generate a random number between 1 and 20 inclusive
    srand(time(NULL) + getpid());
    int number = (rand()%20)+1;
    printf("Random number is: %d\t", number);
    char snum[256];
    sprintf(snum, "<!DOCTYPE html><html><head><title>d20</title></head><body><h1>%d</h1></body></html>", number);
    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////

    // Use send_response() to send it back as text/plain data
    send_response(fd, "HTTP/1.1 200 OK", "text/html", snum, strlen(snum));
    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
}

/**
 * Send a 404 response
 */
void resp_404(int fd)
{
    char filepath[4096];
    struct file_data *filedata;
    char *mime_type;

    // Fetch the 404.html file
    snprintf(filepath, sizeof filepath, "%s/404.html", SERVER_FILES);
    filedata = file_load(filepath);

    if (filedata == NULL) {
        // TODO: make this non-fatal
        fprintf(stderr, "cannot find system 404 file\n");
        exit(3);
    }

    mime_type = mime_type_get(filepath);

    printf("mime_type:%s\n", mime_type);
    printf("filedata->data:%s\n", filedata->data);
    printf("filedata->size:%d\n", filedata->size);

    send_response(fd, "HTTP/1.1 404 NOT FOUND", mime_type, filedata->data, filedata->size);

    file_free(filedata);
}

/**
 * Read and return a file from disk or cache
 */
void get_file(int fd, struct cache *cache, char *request_path)
{
    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
    char filepath[4096];
    struct file_data *filedata;
    printf("request_path:%s\n", request_path);
    snprintf(filepath, sizeof filepath, "./serverroot/%s", request_path);
    struct cache_entry *entry = cache_get(cache, filepath);
    if(entry == NULL){
        filedata = file_load(filepath);
        if (filedata == NULL) {
            // TODO: make this non-fatal
            fprintf(stderr, "cannot find system 404 file\n");
            resp_404(fd);
            return;
        }
        char *content_type = "file";
        printf("Sending uncached file.\n");
        cache_put(cache, filepath, content_type, filedata->data, filedata->size);
    }else{
        printf("Sending cached file.\n");
        filedata = malloc(sizeof *filedata);
        filedata->data = entry->content;
        filedata->size = entry->content_length;
    }
    printf("Sending response.\n");
    send_response(fd, "HTTP/1.1 200 OK", mime_type_get(filepath), filedata->data, filedata->size);
}

/**
 * Search for the end of the HTTP header
 * 
 * "Newlines" in HTTP can be \r\n (carriage return followed by newline) or \n
 * (newline) or \r (carriage return).
 */
char *find_start_of_body(char *header)
{
    ///////////////////
    // IMPLEMENT ME! // (Stretch)
    ///////////////////
    char *p = header;
    for(int i=0; i<3; ){
        if(*p++ == '\r'){
            if(*p == '\n'){
                p++;
            }
            i++;
        }
        if(*p == '\n'){
            i++;
        }
    }
    return p;
}

/**
 * Handle HTTP request and send response
 */
void handle_http_request(int fd, struct cache *cache)
{
    const int request_buffer_size = 65536*4; // 64K
    char request[request_buffer_size];

    // Read request
    int bytes_recvd = recv(fd, request, request_buffer_size - 1, 0);

    if (bytes_recvd < 0) {
        perror("recv");
        return;
    }

    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////

    // Read the three components of the first request line
    char request_type[5];
    char request_path[256];
    char request_protocol[10];
    sscanf(request, "%s %s %s", request_type, request_path, request_protocol);
    // If GET, handle the get endpoints
    if(!strcmp("GET", request_type)){
    //    Check if it's /d20 and handle that special case
        if(!strcmp("/d20", request_path)){
            get_d20(fd);
        }
    //    Otherwise serve the requested file by calling get_file()
        else{
            get_file(fd, cache, request_path);
        }
    // (Stretch) If POST, handle the post request
    }else if(!strcmp("POST", request_type)){
        printf("Got POST request: %s\n", request_path);
        char final_path[4096];
        snprintf(final_path, sizeof final_path, "./serverroot%s", request_path);
        int new_file = open(final_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        write(new_file, find_start_of_body(request), strlen(find_start_of_body(request)));
        printf("%s is %lu.\n", find_start_of_body(request), strlen(find_start_of_body(request)));
        close(new_file);
        send(fd, "application/json 200 OK {\"status\":\"ok\"}", 15, 0);
    }
    else{
        fprintf(stderr, "unknown request type \"%s\"\n", request_type);
        resp_404(fd);
    }


}

/**
 * Main
 */
int main(void)
{
    int newfd;  // listen on sock_fd, new connection on newfd
    struct sockaddr_storage their_addr; // connector's address information
    char s[INET6_ADDRSTRLEN];

    struct cache *cache = cache_create(10, 0);

    // Get a listening socket
    int listenfd = get_listener_socket(PORT);

    if (listenfd < 0) {
        fprintf(stderr, "webserver: fatal error getting listening socket\n");
        exit(1);
    }


    printf("webserver: waiting for connections on port %s...\n", PORT);

    // This is the main loop that accepts incoming connections and
    // forks a handler process to take care of it. The main parent
    // process then goes back to waiting for new connections.
    
    while(1) {
        socklen_t sin_size = sizeof their_addr;

        // Parent process will block on the accept() call until someone
        // makes a new connection:
        newfd = accept(listenfd, (struct sockaddr *)&their_addr, &sin_size);
        if (newfd == -1) {
            perror("accept");
            continue;
        }

        // Print out a message that we got the connection
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);
        
        // newfd is a new socket descriptor for the new connection.
        // listenfd is still listening for new connections.

        handle_http_request(newfd, cache);

        close(newfd);
    }

    // Unreachable code

    return 0;
}

