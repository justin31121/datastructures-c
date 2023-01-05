#ifndef HTTP_H
#define HTTP_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/conf.h>
//link with -lssl -lcrypto

#ifdef _WIN32
#include <winsock2.h>
//link with -lws2_32
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
#ifdef _WIN32
  SOCKET socket;
#endif //_WIN32
  
#ifdef linux  
  int socket;
#endif //linux
  
  SSL_CTX *ctx;
}Http;

Http *http_init();
bool http_get(Http *http, const char* url, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);
void http_close(Http *http);

#ifdef HTTP_IMPLEMENTATION

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

Http *http_init() {
  Http *http = (Http *) malloc(sizeof(Http));
  if(!http) return free_n_null(http);

#ifdef _WIN32
  WSADATA wsaData;
  if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
    return free_n_null(http);
  }

  http->socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
  if(http->socket == INVALID_SOCKET) {
    WSACleanup();
    return free_n_null(http);
  }
#endif //_WIN32

#ifdef linux
  struct protoent *protoent = getprotobyname("tcp");
  if(!protoent) return free_n_null(http);
  
  http->socket = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
  if(http->socket==-1) return free_n_null(http);
#endif //linux

  http->ctx = NULL;
  
  return http;
}

bool http_init_ssl(Http *http) {
  if(!http) return false;

  //SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  //SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  if(!ctx) return false;

  http->ctx = ctx;

  return true;
}

#ifdef _WIN32
bool build_address(const char* hostname, size_t hostname_len,
		   SOCKADDR_IN* addr,
		   bool ssl) {

  char name[hostname_len+1];
  memcpy(name, hostname, hostname_len);
  name[hostname_len] = 0;
  
  struct hostent *hostent = gethostbyname(name);
  if(!hostent) return 1;

  /*
  in_addr_t in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
  if(in_addr == (in_addr_t) -1) return 1;
  */

  addr->sin_addr.s_addr = *((unsigned long*) hostent->h_addr);
  addr->sin_family = AF_INET;
  addr->sin_port = htons(ssl ? HTTPS_PORT : HTTP_PORT);
    
  return true;
}
#endif //_WIN32

#ifdef linux
bool build_address(const char* hostname, size_t hostname_len,
		   struct sockaddr_in* addr,
		   bool ssl) {

  char name[hostname_len+1];
  memcpy(name, hostname, hostname_len);
  name[hostname_len] = 0;
  
  struct hostent *hostent = gethostbyname(name);
  if(!hostent) return 1;

  in_addr_t in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
  if(in_addr == (in_addr_t) -1) return 1;

  addr->sin_addr.s_addr = in_addr;
  addr->sin_family = AF_INET;
  addr->sin_port = htons(ssl ? HTTPS_PORT : HTTP_PORT);
    
  return true;
}
#endif //linux

//TODO: Recognize Transfer-Encoding: Chunked
bool http_get(Http *http, const char* url, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata) {
  
  char buffer[BUFSIZ];
  if(!http) {
    printf("!http\n");
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
    if(!http->ctx) {
      if(!http_init_ssl(http)) {
	printf("http_init_ssl\n");
	return false;
      }
    }

    conn = SSL_new(http->ctx);
    if(!conn) {
      printf("SSL_new\n");
      return false;
    }

    SSL_set_fd(conn, http->socket);
  }

  //BUILD HTTPUEST
#define MAX_HTTPUEST_LEN 1024
  char httpuest[MAX_HTTPUEST_LEN];

  const char *route = "/";
  if(directory_len>0) {
    route = url + hostname + hostname_len;
  }

  int httpuest_len = snprintf(httpuest, MAX_HTTPUEST_LEN,
			     "GET %s HTTP/1.1\r\n"
			     "Host: %.*s\r\n"
			     "User-Agent: PostmanRuntime/7.28.1\r\n"
			     "DNT: 1\r\n"
			     "Connection: close\r\n"
			     "Pragma: no-cache\r\n"
			     "Cache-Control: no-cache\r\n"
			     "\r\n", route, (int) hostname_len, url + hostname);

  if(httpuest_len >= MAX_HTTPUEST_LEN) {
    printf("httpuest_len >= MAX_HTTPUEST_LEN\n");
    return false; 
  }

#ifdef HTTP_DEBUG
  printf("%s\n", httpuest);
#endif //HTTP_DEBUG

  //BUILD ADDRESS
#ifdef _WIN32
  SOCKADDR_IN addr;
#endif //WIN32
#ifdef linux
  struct sockaddr_in addr;
#endif //linux  
  if(!build_address(url + hostname, hostname_len, &addr, ssl)) {
    printf("build_address\n");
    return false; 
  }

  //CONNECT
#ifdef _WIN32
  if(connect(http->socket, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
    printf("connect\n");
    return false;
  }
#endif //WIN32
#ifdef linux
  if(connect(http->socket, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
    printf("connect\n");
    return false;
  }
#endif //linux 

  if(ssl) {
    int res = SSL_connect(conn);
    if(res!=1) {     
      //int error = SSL_get_error(conn, res);
      //print_SSL_error(error);
      printf("SSL_connect\n");
      return false;
    }
  }

  ssize_t nbytes_last;
  ssize_t nbytes_total = 0;
  while(nbytes_total < httpuest_len) {
    if(ssl) {
      nbytes_last = SSL_write(conn, httpuest + nbytes_total, httpuest_len - nbytes_total);
    }
    else {
#ifdef _WIN32
      nbytes_last = send(http->socket, httpuest + nbytes_total, httpuest_len - nbytes_total, 0);
#endif //WIN32
#ifdef linux
      nbytes_last = read(http->socket, buffer, BUFSIZ);
#endif //linux
    }
    if(nbytes_last == -1) {
      printf("write\n");
      return false;
    }
    nbytes_total += nbytes_last;
  }

  int inHeader = -1;
  bool first = true;

  char window[4] = {0};
  int s = 0;

  do{
    if(ssl) {
      nbytes_total = SSL_read(conn, buffer, BUFSIZ);
    }
    else {
#ifdef _WIN32
      nbytes_total = recv(http->socket, buffer, BUFSIZ, 0);
#endif //WIN32
#ifdef linux
      nbytes_total = read(http->socket, buffer, BUFSIZ);
#endif //linux      
    }

    size_t offset = 0;
    if(inHeader == -1) {
      if(first) {
	memcpy(window, buffer, 4);
	first = false;
      }
      
      for(int i=0;i<BUFSIZ;i++) {
	if(window[s]=='\r' && window[(s+1)%4]=='\n' &&
	   window[(s+2)%4]=='\r' && window[(s+3)%4]=='\n') {
	  inHeader = i;
	  break;
	}
	window[s] = buffer[i];
	s = (s+1) % 4;
      }

      offset = inHeader;
    }

    if(inHeader == -1) {
      continue;
    }    

    if(write_callback) {
      write_callback(buffer + offset, nbytes_total - offset, 1, userdata); 
    }        
  }while(nbytes_total > 0);

  if(nbytes_total == -1) {
    printf("read\n");
    return false;
  }

  if(ssl) SSL_free(conn);
  
  return true;
}

void http_close(Http *http) {
  if(!http) return;
#ifdef _WIN32
  closesocket(http->socket);
  WSACleanup();
#endif //_WIN32

#ifdef linux
  if(http->socket!=-1) close(http->socket);
#endif //linux
  if(http->ctx) SSL_CTX_free(http->ctx);
  free(http);
}

#endif //HTTP_IMPLEMENTATION

#endif //HTTP_H
