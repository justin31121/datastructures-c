#ifndef REQ_H
#define REQ_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/conf.h>
#include <unistd.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif //_WIN32

#ifdef linux
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif //linux

#define HTTP_PORT 80
#define HTTPS_PORT 443

typedef struct{
  int socket;
  SSL_CTX *ctx;
}Req;

Req *req_init();
bool req_get(Req *req, const char* url, uint8_t *response);
void req_close(Req *req);

#ifdef REQ_IMPLEMENTATION

int find_string_len(const char* string, size_t string_len, size_t offset, const char* word, size_t word_len) {  
  for(size_t i=offset; i < (string_len - word_len + 1); ++i) {
    bool found = true;
    for(size_t j=0;j<word_len;j++) {
      if(string[i+j] != word[j]) {
	found = false;
	break;
      }      
    }
    if(found) {
      return i;
    }
  }

  return -1;
}

int find_string(const char* string, size_t offset, const char* word) {
  return find_string_len(string, strlen(string), offset, word, strlen(word));
}

int find_hostname(const char *url, size_t url_len,
		  size_t *hostname_len, bool *ssl) {
  char *http = "http://";
  size_t http_len = 7;
  char* https = "https://";
  size_t https_len = 8;
    
  int start = find_string_len(url, url_len, 0, http, http_len);
  if(start<0) {
    start = find_string_len(url, url_len, 0, https, https_len);
    if(start<0) return -1;
    start += https_len;
    if(ssl) *ssl = true;
  }
  else {
    start += http_len;
    if(ssl) *ssl = false;
  }

  int end = find_string_len(url, url_len, start, "/", 1);
  if(end<0) end = url_len;

  if(hostname_len) *hostname_len = end - start;
  return start;
}

void *free_n_null(void *ptr) {
  if(ptr) free(ptr);
  return NULL;
}

Req *req_init() {
  Req *req = (Req *) malloc(sizeof(Req));
  if(!req) return free_n_null(req);

  struct protoent *protoent = getprotobyname("tcp");
  if(!protoent) return free_n_null(req);
  
  req->socket = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
  if(req->socket==-1) return free_n_null(req);

  req->ctx = NULL;
  
  return req;
}

bool req_init_ssl(Req *req) {
  if(!req) return false;

  //SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  //SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  if(!ctx) return false;

  req->ctx = ctx;

  return true;
}

bool build_address(const char* hostname, size_t hostname_len,
		   struct sockaddr_in* addr, bool ssl) {

  char name[hostname_len+1];
  memcpy(name, hostname, hostname_len);
  name[hostname_len] = 0;

  printf("hostname: %s\n", name);
  
  struct hostent *hostent = gethostbyname(name);
  if(!hostent) return 1;

#ifdef _WIN32
  InetPton(AF_INET, _T(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list))), &addr->sin_addr.s_addr);
#endif //_WIN32

#ifdef linux
  in_addr_t in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
  if(in_addr == (in_addr_t) -1) return 1;

  addr->sin_addr.s_addr = in_addr;
  addr->sin_family = AF_INET;
  addr->sin_port = htons(ssl ? HTTPS_PORT : HTTP_PORT);
#endif
    
  return true;
}

bool req_get(Req *req, const char* url, uint8_t *response) {
  (void) response;
  
  char buffer[BUFSIZ];
  if(!req) {
    printf("!req\n");
    return false;
  }

  size_t hostname_len, directory_len;
  size_t url_len = strlen(url);
  bool ssl;
  int hostname = find_hostname(url, url_len, &hostname_len, &ssl);

  if(hostname<0) {
    printf("hostname<0\n");
    return false;
  }
  directory_len = url_len - hostname - hostname_len;

  SSL *conn = NULL;
  if(ssl) {
    //MAYBE INIT
    if(!req->ctx) {
      if(!req_init_ssl(req)) {
	printf("req_init_ssl\n");
	return false;
      }
    }

    conn = SSL_new(req->ctx);
    if(!conn) {
      printf("SSL_new\n");
      return false;
    }

    SSL_set_fd(conn, req->socket);
  }

  //BUILD REQUEST
#define MAX_REQUEST_LEN 1024
  char request[MAX_REQUEST_LEN];
  int i = 0;
  memcpy(request + i, "GET ", 4); i+=4;
  if(directory_len<=0) {
    request[i++] = '/';
  }
  else {    
    memcpy(request + i, url + hostname + hostname_len, directory_len);
    i+=directory_len;
  }
  char* http = " HTTP/1\r\nHost: ";
  size_t http_len = strlen(http);
  memcpy(request + i, http, http_len); i+=http_len;
  memcpy(request + i, url + hostname, hostname_len); i+=hostname_len;
  memcpy(request + i, "\r\n", 2); i+=2;  
  memcpy(request + i, "User Agent: Mozilla/5.0 (Windows NT 10.0; Win64; rv:104.0) Gecko/20100101 Firefox/14.0\r\n", 88); i+=88;
  memcpy(request + i, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n", 95); i+=95;
  memcpy(request + i, "Accept-Language: de,en.US;q=0.7,en;q=0.3\r\n", 42); i+= 42;
  memcpy(request + i, "DNT: 1\r\n", 8); i+=8;  
  char *connection = "Connection: close\r\n";
  size_t connection_len = strlen(connection);
  memcpy(request + i, connection, connection_len); i+=connection_len;
  memcpy(request + i, "Pragma: no-cache\r\n", 18); i+=18;
  memcpy(request + i, "Cache-Control: no-cache\r\n", 25); i+=25;
  memcpy(request + i, "\r\n", 2); i+=2;
  if(i<MAX_REQUEST_LEN-1) {
    request[i] = 0;
    printf("%s\n", request);
  }
  ssize_t request_len = i;
  if(request_len >= MAX_REQUEST_LEN) {
    printf("request_len >= MAX_REQUEST_LEN\n");
    return false; 
  }

  //BUILD ADDRESS
  struct sockaddr_in addr;
  if(!build_address(url + hostname, hostname_len, &addr, ssl)) {
    printf("build_address\n");
    return false; 
  }

  //CONNECT
  if(connect(req->socket, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
    printf("connect\n");
    return false;
  }

  if(ssl) {
    int res = SSL_connect(conn);
    if(res!=1) {     
      //int error = SSL_get_error(conn, res);
      //print_SSL_error(error);
      printf("SSL_connect\n");
      return false;
    }
  }

  //TODO: SEND RESPONSE
  ssize_t nbytes_last;
  ssize_t nbytes_total = 0;
  while(nbytes_total < request_len) {
    if(ssl) {
      nbytes_last = SSL_write(conn, request + nbytes_total, request_len - nbytes_total);
    }
    else {
      nbytes_last = write(req->socket, request + nbytes_total, request_len - nbytes_total);
    }
    if(nbytes_last == -1) {
      printf("write\n");
      return false;
    }
    nbytes_total += nbytes_last;
  }
  
  //TODO: READ RESPONSE
  do{
    if(ssl) {
      nbytes_total = SSL_read(conn, buffer, BUFSIZ);
    }
    else {
      nbytes_total = read(req->socket, buffer, BUFSIZ);
    }
    write(STDOUT_FILENO, buffer, nbytes_total);
  }while(nbytes_total > 0);

  if(nbytes_total == -1) {
    printf("read\n");
    return false;
  }

  if(ssl) SSL_free(conn);
  return true;
}

void req_close(Req *req) {
  if(!req) return;
  if(req->socket!=-1) close(req->socket);  
  if(req->ctx) SSL_CTX_free(req->ctx);
  free(req);
}

#endif //REQ_IMPLEMENTATION

#endif //REQ_H
