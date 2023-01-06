#ifndef HTTP_H
#define HTTP_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

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
#include <fcntl.h>
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
bool http_request(Http *http, const char *url, const char *method, const char* body, const char *content_type, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);
bool http_get(Http *http, const char* url, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);
bool http_post(Http *http, const char *url, const char *body, const char *content_type, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);
void http_close(Http *http);

//BEGIN HTTP_SERVER

typedef struct{
#ifdef _WIN32
  SOCKET socket;
#endif //_WIN32
  
#ifdef linux  
  int socket;
#endif //linux
}HttpServer;

typedef enum {
  HTTP_ACCEPT_OK = 0,
  HTTP_ACCEPT_AGAIN,
  HTTP_ACCEPT_ERROR
}HttpAccept;

HttpServer *http_server_init(int port);
void http_server_close(HttpServer *server);

//END HTTP_SERVER

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

HttpAccept http_accept(int socket, int *res_client) {  
  struct sockaddr_in addr;

#ifdef _WIN32
  int addr_len = sizeof(addr);
  SOCKET client = accept(socket, (struct sockaddr *) &addr, &addr_len);
  if(client == INVALID_SOCKET) {
    if(WSAGetLastError() == WSAEWOULDBLOCK) {
      return HTTP_ACCEPT_AGAIN;
    }
    return HTTP_ACCEPT_ERROR;
  } 
#endif //_WIN32
#ifdef linux
  socklen_t addr_len = sizeof(addr);
  int client = accept(socket, (struct sockaddr *) &addr, &addr_len);
  if(client == -1) {
    if(errno == EAGAIN || errno == EWOULDBLOCK) {
      return HTTP_ACCEPT_AGAIN;
    }
    return HTTP_ACCEPT_ERROR;
  }
#endif //linux  

  *res_client = client;
  return HTTP_ACCEPT_OK;
}

#define HTTP_READ_BUFFER_CAP BUFSIZ

bool http_read_header(int socket, SSL *conn, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata) {
  if(socket == -1) {
    return false;
  }
  char buffer[HTTP_READ_BUFFER_CAP];

  int inHeader = -1;
  //bool first = true;

  char window[4] = {0};
  int s = 0;

  ssize_t nbytes_total = 0;
  ssize_t nbytes_last;
  do {
    if(conn != NULL) {
      nbytes_last = SSL_read(conn, buffer, HTTP_READ_BUFFER_CAP);
    }
    else {
#ifdef _WIN32
      nbytes_last = recv(socket, buffer, HTTP_READ_BUFFER_CAP, 0);
#endif //WIN32
#ifdef linux
      nbytes_last = read(socket, buffer, HTTP_READ_BUFFER_CAP);
#endif //linux      
    }

    printf("Read: %lld\n", nbytes_last);

    if(nbytes_last <= 0) {
      return false;
    }
    
    //printf("[INFO]: Read %ld bytes\n", nbytes_last);
    if(nbytes_last!=0 && write_callback != NULL) {
      if(write_callback) {
	write_callback(buffer, nbytes_last, 1, userdata);
      }
      //printf("%.*s", (int) nbytes_last, buffer);
    }

    //if(first) {
    //memcpy(window, buffer, 4);
    //first = false;
    //}
      
    for(int i=0;i<HTTP_READ_BUFFER_CAP;i++) {
      if(window[s]=='\r' && window[(s+1)%4]=='\n' &&
	 window[(s+2)%4]=='\r' && window[(s+3)%4]=='\n') {
	inHeader = i;
	break;
      }
      window[s] = buffer[i];
      s = (s+1) % 4;
    }

    nbytes_total += inHeader == -1 ? nbytes_last : inHeader;
    if(inHeader != -1) break;
  }while(nbytes_last > 0);

  return true;
}

bool http_read_body(int socket, SSL *conn, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata) {
  if(socket == -1) {
    return false;
  }  
  
  char buffer[HTTP_READ_BUFFER_CAP];
  
  int inHeader = -1;
  bool first = true;

  char window[4] = {0};
  int s = 0;

  ssize_t nbytes_total;
  do{
    if(conn != NULL) {
      nbytes_total = SSL_read(conn, buffer, HTTP_READ_BUFFER_CAP);
    }
    else {
#ifdef _WIN32
      nbytes_total = read(socket, buffer, HTTP_READ_BUFFER_CAP);
#endif //WIN32
#ifdef linux
      nbytes_total = read(socket, buffer, HTTP_READ_BUFFER_CAP);
#endif //linux      
    }

    if(nbytes_total == -1) {
      return false;
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

#ifdef HTTP_DEBUG
      printf("%.*s", (inHeader == -1 ? HTTP_READ_BUFFER_CAP : inHeader), buffer);
#endif
    }

    if(inHeader == -1) {
      continue;
    }

    if(write_callback) {
      write_callback(buffer + offset, nbytes_total - offset, 1, userdata); 
    }        
  }while(nbytes_total > 0);

  return true;
}

bool http_send_len(int socket, SSL *conn, const char *buffer, ssize_t buffer_size) {
  if(socket == -1) return false;
  if(buffer == NULL) return false;
  if(buffer_size == 0) {
    printf("[WARNING] You are trying to send 0 bytes");
  }

  ssize_t nbytes_last;
  ssize_t nbytes_total = 0;
  while(nbytes_total < buffer_size) {
    if(conn != NULL) {
      nbytes_last = SSL_write(conn, buffer + nbytes_total, buffer_size - nbytes_total);
    } else {
      nbytes_last = send(socket, buffer + nbytes_total, buffer_size - nbytes_total, 0);
    }
    if(nbytes_last == -1) {
      return false;
    }
    nbytes_total += nbytes_last;
  }
  
  return true;
}

bool http_send(int socket, SSL *conn, const char *buffer) {
  return http_send_len(socket, conn, buffer, strlen(buffer));
}

bool http_connect(int socket, bool ssl, const char* hostname, size_t hostname_len) {
  if(socket == -1) {
    return false;
  }
  if(hostname == NULL) {
    return false;
  }
  if(hostname_len == 0) {
    printf("[WARNING] You are trying to connect a host withtout a name. The hostname_len is 0.");
    return true;
  }

#ifdef _WIN32
  SOCKADDR_IN addr;
#endif //WIN32
#ifdef linux
  struct sockaddr_in addr;
#endif //linux

  //HOSTNAME to Cstr
  char name[hostname_len+1];
  memcpy(name, hostname, hostname_len);
  name[hostname_len] = 0;

  struct hostent *hostent = gethostbyname(name);
  if(!hostent) return false;
   
#ifdef _WIN32
  addr.sin_addr.s_addr = *((unsigned long*) hostent->h_addr);
#endif //WIN32
#ifdef linux
  in_addr_t in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
  if(in_addr == (in_addr_t) -1) return false;
  addr.sin_addr.s_addr = in_addr;
#endif //linux  
  addr.sin_family = AF_INET;
  addr.sin_port = htons(ssl ? HTTPS_PORT : HTTP_PORT);

  if(connect(socket, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
    return false;
  }
  
  return true;
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

//TODO: Recognize Transfer-Encoding: Chunked
bool http_request(Http *http, const char *url, const char *method, const char* body, const char *content_type, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata) {
  
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

  //BUILD REQUEST
#define MAX_REQUEST_LEN 1024
  char request[MAX_REQUEST_LEN];

  const char *route = "/";
  if(directory_len>0) {
    route = url + hostname + hostname_len;
  }

  const char *request_body = body == NULL ? "" : body;
  const char *request_content_type = content_type == NULL ? "text/plain" : content_type;
  const int request_content_length = body == NULL ? -1 : (int) strlen(body);

  int request_len;
  if(body == NULL && content_type == NULL) {
    request_len = snprintf(request, MAX_REQUEST_LEN,
			   "%s %s HTTP/1.1\r\n"
			   "Host: %.*s\r\n"			      
			   "DNT: 1\r\n"
			   "Connection: close\r\n"
			   "Pragma: no-cache\r\n"
			   "Cache-Control: no-cache\r\n"
			   "\r\n", method, route, (int) hostname_len, url + hostname);
  } else {
    request_len = snprintf(request, MAX_REQUEST_LEN,
			   "%s %s HTTP/1.1\r\n"
			   "Host: %.*s\r\n"			      
			   "DNT: 1\r\n"
			   "Connection: close\r\n"
			   "Pragma: no-cache\r\n"
			   "Cache-Control: no-cache\r\n"
			   "Content-Type: %s\r\n"			     
			   "Content-Length: %d\r\n"
			   "\r\n"
			   "%s", method, route, (int) hostname_len, url + hostname,
			   request_content_type,
			   request_content_length,
			   request_body);    
  }



  //TODO: If too big allocate somthing on the heap
  if(request_len >= MAX_REQUEST_LEN) {
    printf("httpuest_len >= MAX_HTTPUEST_LEN\n");
    return false; 
  }

#ifdef HTTP_DEBUG
  printf("%s\n", request);
#endif //HTTP_DEBUG

  //CONNECT
  if(!http_connect(http->socket, ssl, url + hostname, hostname_len)) {
    return false;
  }

  //SSL CONNECT
  if(ssl) {
    int res = SSL_connect(conn);
    if(res!=1) {     
      //int error = SSL_get_error(conn, res);
      //print_SSL_error(error);
      printf("SSL_connect\n");
      return false;
    }
  }

  //SEND
  if(!http_send_len(http->socket, conn, request, request_len)) {
    return false;
  }

  //READ
  if(!http_read_body(http->socket, conn, write_callback, userdata) ) {
    return false;
  }

  if(ssl) SSL_free(conn);
  
  return true;
}

bool http_get(Http *http, const char* url, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata) {
  return http_request(http, url, "GET", NULL, NULL, write_callback, userdata);
}

bool http_post(Http *http, const char *url,
	       const char *body, const char *content_type,
	       size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata) {
  return http_request(http, url, "POST", body, content_type, write_callback, userdata);
}

void http_close(Http *http) {
  if(!http) return;
#ifdef _WIN32
  if(http->socket != INVALID_SOCKET) {
    closesocket(http->socket);
    WSACleanup();
  }
#endif //_WIN32

#ifdef linux
  if(http->socket!=-1) close(http->socket);
#endif //linux
  if(http->ctx) SSL_CTX_free(http->ctx);
  free(http);
}

//BEGIN HTTP_SERVER
HttpServer *http_server_init(int port) {
  HttpServer *server = (HttpServer *) malloc(sizeof(HttpServer));
  if(!server) return free_n_null(server);
  
#ifdef _WIN32
  WSADATA wsaData;
  if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
    return free_n_null(server);
  }

  server->socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
  if(server->socket == INVALID_SOCKET) {
    WSACleanup();
    return free_n_null(server);
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if(bind(server->socket, (SOCKADDR*) &addr, sizeof(addr)) == SOCKET_ERROR) {
    closesocket(server->socket);
    WSACleanup();
    return free_n_null(server);
  }

  unsigned long mode = 1;
  ioctlsocket(server->socket, FIONBIO, &mode);
#endif //_WIN32

#ifdef linux
  struct protoent *protoent = getprotobyname("tcp");
  if(!protoent) return free_n_null(server);
  
  server->socket = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
  if(server->socket==-1) return free_n_null(server);

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if(bind(server->socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    close(server->socket);
    return free_n_null(server);
  }

  int flags = fcntl(server->socket, F_GETFL, 0);
  fcntl(server->socket, F_SETFL, flags | O_NONBLOCK);
#endif //linux
  
  return server;
}

void http_server_close(HttpServer *server) {
  if(!server) return;
  
#ifdef _WIN32
  if(server->socket != INVALID_SOCKET) {
    closesocket(server->socket);
    shutdown(server->socket, SD_BOTH);
    WSACleanup();
  }
#endif //_WIN32

#ifdef linux
  if(server->socket!=-1) {
    close(server->socket);
    shutdown(server->socket, SHUT_RDWR);
  }
#endif //linux
  
  free(server);
}
//END HTTP_SERVER

#endif //HTTP_IMPLEMENTATION

#endif //HTTP_H
