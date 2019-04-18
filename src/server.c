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
/* send_response(fd, "HTTP/1.1 404 NOT FOUND", mime_type, filedata->data, filedata->size); */
int send_response(int fd, char *header, char *content_type, void *body, int content_length)
{
  /* printf("calling send response\n"); */
    const int max_response_size = 262144;
    char response[max_response_size];
    time_t rawtime;
    struct tm *info;
    time( &rawtime );
    info = localtime( &rawtime );
    /* printf("local %d", info); */
    /* printf("response: %c", response); */
    int response_length = sprintf(response, 
        "%s\n"
        "Date: %s"
        "Connection: close\n"
        "Content-Type: %s\n"
        "Content-Length: %d\n"
        "\n",
        /* "%s", */
       header,
       asctime(info),
       content_type,
       content_length);
    /* shifts over response length bytes. Then adds body to the end. */
    memcpy(response + response_length, body, content_length);

    printf("****** Beginning of response ********\n");
    printf("%s\n", response);
    printf("****** End of response ********\n");
    int rv = send(fd, response, response_length + content_length, 0);
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
  char *mime_type = "text/plain";
  srand(time(NULL));
  int n = rand() % 20 + 1;
  /* int size_of_int = sizeof(n); */
  char d20[16];
  sprintf(d20, "%d", n);
  send_response(fd, "HTTP/1.1 200 OK", mime_type, d20, strlen(d20));
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
    /* snprintf(filepath, sizeof filepath, "./serverfiles/404.html"); */
    filedata = file_load(filepath);
    printf("filepath: %s\n", filepath);
    /* printf("filedata: %d\n", filedata); */

    if (filedata == NULL) {
        // TODO: make this non-fatal
        printf("This is the error\n");
        fprintf(stderr, "cannot find system 404 file\n");
        exit(3);
    }
    /* } else { */
    /*   printf("This means the file data is not null but it's something else\n"); */
    /*   exit(3); */
    /* } */

    mime_type = mime_type_get(filepath);

    send_response(fd, "HTTP/1.1 404 NOT FOUND", mime_type, filedata->data, filedata->size);

    file_free(filedata);
}

/**
 * Read and return a file from disk or cache
 */
void get_file(int fd, struct cache *cache, char *request_path)
{

  printf(" 142: requests path: %s\n", request_path);
  char filepath[4096];
  struct file_data *filedata;
  char *mime_type;
  snprintf(filepath, sizeof filepath, "%s%s", SERVER_ROOT, request_path);

  filedata = file_load(filepath);
  printf(" 149: file path: %s\n", filepath);
  printf(" 150: file data: %d\n", filedata);

  if(filedata == NULL) {
    /* printf("going to 404\n"); */
    resp_404(fd);
  }

  mime_type = mime_type_get(filepath);
  send_response(fd, "HTTP/1.1 200 OK", mime_type, filedata->data, filedata->size );
}

/**
 * Search for the end of the HTTP header
 *
 * "Newlines" in HTTP can be \r\n (carriage return followed by newline) or \n
 * (newline) or \r (carriage return).
 */
/* char *find_start_of_body(char *header) */
/* { */
/*     /////////////////// */
/*     // IMPLEMENT ME! // (Stretch) */
/*     /////////////////// */
/* } */

/**
 * Handle HTTP request and send response
 */
void handle_http_request(int fd, struct cache *cache)
{
    const int request_buffer_size = 65536; // 64K
    char request[request_buffer_size];
    char method[200];
    char path[2048];
    /* char thing[2048]; */

    // Read request
    int bytes_recvd = recv(fd, request, request_buffer_size - 1, 0);
    sscanf(request, "%s %s", method, path);
    /* printf("thing: %s\n", thing); */
    /* printf("method: %s\n", method); */
    printf("path: %s\n", path);

    if(strcmp(method, "GET") == 0) {
      if(strcmp(path, "/d20") == 0) {
        get_d20(fd);
      } else if (strcmp(path, "/") == 0)  {
        get_file(fd, cache, "/index.html");
      } else {
        /* server 404 if none of theother cases work */
        get_file(fd, cache, path);

      }
    } else if (strcmp(method, "POST" == 0)) {
      printf("the method is %s\n", method);
    }

    if (bytes_recvd < 0) {
        perror("recv");
        return;
    }


    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////

    // Read the first two components of the first line of the request 
    // so parse the request. Check for the http methods.
    // Respond accordingly.

    // If GET, handle the get endpoints

    //    Check if it's /d20 and handle that special case
    //    Otherwise serve the requested file by calling get_file()


    // (Stretch) If POST, handle the post request
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
    // responds to the request. The main parent process
    // then goes back to waiting for new connections.

    while(1) {
        socklen_t sin_size = sizeof their_addr;

        // Parent process will block on the accept() call until someone
        // makes a new connection:
        newfd = accept(listenfd, (struct sockaddr *)&their_addr, &sin_size);
        if (newfd == -1) {
            perror("accept");
            continue;
        }
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

