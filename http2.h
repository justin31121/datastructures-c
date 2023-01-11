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

#ifdef HTTP_IMPLEMENTATION

#ifndef STRING_IMPLEMENTATION
#define STRING_IMPLEMENTATION
#endif //STRING_IMPLEMENTATION

#endif //HTTP_IMPLEMENTATION
#include "./string.h"

#ifdef HTTP_IMPLEMENTATION

#ifndef UTIL_IMPLEMENTATION
#define UTIL_IMPLEMENTATION
#endif //UTIL_IMPLEMENTATION

#endif //HTTP_IMPLEMENTATION
#include "./util.h"

#define HTTP_PORT 80
#define HTTPS_PORT 443

size_t _fwrite(const void *data, size_t size, size_t memb, void *userdata) {
  return fwrite(data, size, memb, (FILE *) userdata);
}

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
  int socket;
  bool used;
  String_Buffer buffer;
  pthread_t id;
  void (*handle_request)(string method, string path, string headers, string body, int client);
}HttpServerThread;

typedef struct{
#ifdef _WIN32
  SOCKET socket;
#endif //_WIN32
  
#ifdef linux  
  int socket;
#endif //linux

  HttpServerThread *threads;
  size_t threads_count;

  pthread_t listen_thread_id;
  bool listen_thread_stop;
  
}HttpServer;

typedef enum {
  HTTP_ACCEPT_OK = 0,
  HTTP_ACCEPT_AGAIN,
  HTTP_ACCEPT_ERROR
}HttpAccept;

HttpServer *http_server_init(int port);
void http_server_listen_and_serve(HttpServer *server, void (*handle_request)(string method, string path, string headers, string body, int client), size_t number_of_threads);
void http_server_stop(HttpServer *server);
void http_server_close(HttpServer *server);

void http_server_simple_file_handler(string method, string path, string headers, string body, int client);

typedef struct {
  int code;
  string message;
  string content_type;
  string body;
}HttpResponse;

bool http_response_send(int socket, SSL *conn, HttpResponse http_response);
string http_content_type_for_filename_len(const char *file_path, size_t file_path_len);

//END HTTP_SERVER

#ifdef HTTP_IMPLEMENTATION

int find_hostname(const char *url, size_t url_len,
		  size_t *hostname_len, bool *ssl) {
  char *http = "http://";
  size_t http_len = 7;
  char* https = "https://";
  size_t https_len = 8;
    
  int start = cstr_index_of(url, url_len, http, http_len);
  if(start<0) {
    start = cstr_index_of(url, url_len, https, https_len);
    if(start<0) return -1;
    start += https_len;
    if(ssl) *ssl = true;
  }
  else {
    start += http_len;
    if(ssl) *ssl = false;
  }

  int end = cstr_index_of(url + start, url_len - start, "/", 1);
  if(end<0) end = url_len;

  if(hostname_len) *hostname_len = end - start - 1;
  return start;
}

void *free_n_null(void *ptr) {
  if(ptr) free(ptr);
  return NULL;
}

void http_make_nonblocking(int socket) {
#ifdef _WIN32
  unsigned long mode = 1;
  ioctlsocket(socket, FIONBIO, &mode);
#endif //_WIN32
#ifdef linux
  int flags = fcntl(socket, F_GETFL, 0);
  fcntl(socket, F_SETFL, flags | O_NONBLOCK);
#endif //linux
}

HttpAccept http_select(int client, fd_set *read_fds, struct timeval *timeout) {
  
#ifdef _WIN32
  int ret = select(0, read_fds, NULL, NULL, timeout);
  if(ret == SOCKET_ERROR) {
    return HTTP_ACCEPT_ERROR;
  } else if(ret == 0) {
    return HTTP_ACCEPT_AGAIN;
  }
#endif //_WIN32
#ifdef linux
  errno = 0;
  int ret = select(client + 1, read_fds, NULL, NULL, timeout);
  if(ret == -1) {
    return HTTP_ACCEPT_ERROR;
  } else if(ret == 0) {
    return HTTP_ACCEPT_AGAIN;
  }
#endif //linux
  
  if(!FD_ISSET(client, read_fds)) {
    return HTTP_ACCEPT_AGAIN;
  }
  
  return HTTP_ACCEPT_OK;
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

bool http_read(int socket, SSL *conn, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata) {
  if(socket == -1) {
    return false;
  }
  char buffer[HTTP_READ_BUFFER_CAP];

  /*
  int inHeader = -1;
  //bool first = true;

  char window[4] = {0};
  int s = 0;  
  */

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
      nbytes_last = recv(socket, buffer, HTTP_READ_BUFFER_CAP, 0);
#endif //linux      
    }

#ifdef linux
    if(nbytes_last < 0) {
      if(errno != EAGAIN && errno !=EWOULDBLOCK) {
	return false;
      }
      break;
    }
#endif //linux
#ifdef _WIN32
    if(nbytes_last == SOCKET_ERROR) {
      if(WSAGetLastError() != WSAEWOULDBLOCK) {
	return false;
      }
      break;
    }
#endif //_WIN32

    nbytes_total += nbytes_last;
    
    //printf("[INFO]: Read %ld bytes\n", nbytes_last);
    if(nbytes_last>0 && write_callback != NULL) {
      if(write_callback) {
	write_callback(buffer, nbytes_last, 1, userdata);
      }
      //printf("%.*s", (int) nbytes_last, buffer);
    }

    //if(first) {
    //memcpy(window, buffer, 4);
    //first = false;
    //}
    /*
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
    */
  }while(true);

#ifdef _WIN32
  return nbytes_total!=0 && nbytes_total!=SOCKET_ERROR;
#endif //WIN32
#ifdef linux

  return nbytes_total > 0;
#endif //linux
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
    printf("socket is -1\n");
    return false;
  }
  if(hostname == NULL) {
    printf("hostname is NULL\n");
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
  if(!hostent) {
    printf("hostent\n");
    return false;
  }
   
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

  if(connect(socket, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
    printf("connect\n");
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

  if(http == NULL) {
    printf("http is null");
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
    printf("http_connect\n");
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
    printf("http_send_len\n");
    return false;
  }

  //READ
  if(!http_read_body(http->socket, conn, write_callback, userdata) ) {
    printf("http_read_body\n");
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

  server->threads = NULL;
  server->threads_count = 0;
  server->listen_thread_stop = true;
  
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
#endif //linux

  http_make_nonblocking(server->socket);
  
  return server;
}

void http_close_socket(int socket) {
#ifdef _WIN32
  closesocket(socket);
#endif //_WIN32
#ifdef linux
  close(socket);
#endif //linux
}

#define HTTP_SERVER_THREAD_START(threadid_ptr, thread_function, arg) if(pthread_create(threadid_ptr, NULL, thread_function, arg) != 0)  fprintf(stderr, "[WARNING]: Can not create thread\n")

#define HTTP_SERVER_THREAD_JOIN(threadid) pthread_join(threadid, NULL)

void http_sleep_ms(int ms) {
  struct timespec sleep_time;
  if(ms < 1000) {
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = ms * 1000000;    
  } else {
    sleep_time.tv_sec = ms / 1000;
    sleep_time.tv_nsec = 0;
  }
  if(nanosleep(&sleep_time, NULL) == -1) {
    fprintf(stderr, "[WARNING] Failed to sleep\n");
  }
}

void *http_server_serve_function(void *arg) {
  HttpServerThread *thread = (HttpServerThread *) arg;
  int client = thread->socket;
  bool *used = &(thread->used);
  String_Buffer *buffer = &(thread->buffer);
  void (*handle_request)(string method, string path, string headers, string body, int client) = thread->handle_request;

  http_make_nonblocking(client);

  fd_set read_fds = {0};
  FD_SET(client, &read_fds);
  
  struct timeval timeout;
  timeout.tv_sec = 2;
  timeout.tv_usec = 0;

  //  printf("[INFO] Handling client %d in Thread\n", client);

  while(true) {
    buffer->len = 0;
    HttpAccept select = http_select(client, &read_fds, &timeout); 
    if(select != HTTP_ACCEPT_OK) {
      break;
    }

    if(!http_read(client, NULL, string_buffer_callback, buffer)) {
      break;
    }

    string method = {0};
    string path = {0};

    size_t headersStart = 0;
    size_t bodyStart = 0;

    bool methodFound = 0;
    bool headerFound = 0;
    string request = string_from(buffer->data, buffer->len);
    while(request.len) {
      string line = string_trim(string_chop_by_delim(&request, '\n'));

      if(!methodFound) {
	method = string_trim(string_chop_by_delim(&line, ' '));
	path = string_trim(string_chop_by_delim(&line, ' '));
	methodFound = true;
	continue;
      }

      if(headersStart == 0) {
	headersStart = line.data - buffer->data;
      }

      if(!headerFound) {
	if(line.len == 0) headerFound = true;
	continue;
      }

      if(bodyStart == 0) {
	bodyStart = line.data - buffer->data;
      }


      
      break;
    }

    string body = string_from(buffer->data + bodyStart, buffer->len - bodyStart);
    //printf("BODY: '"String_Fmt"'\n", String_Arg(body));

    string headers = string_trim(string_from(buffer->data + headersStart, buffer->len - headersStart - body.len));
    //printf("HEADERS: '"String_Fmt"'\n", String_Arg(headers));

    if(handle_request != NULL) {
      handle_request(method, path, headers, body, client);
    }
  }

  //  printf("[INFO] Closing connection to client %d in Thread\n", client);  
  http_close_socket(client);
  *used = false;
  
  return NULL;
}

void http_server_create_serve_thread(HttpServer *server, int client) {
  for(size_t i=0;i<server->threads_count;i++) {
    HttpServerThread *thread = &(server->threads[i]);
    if(thread->used==true) continue;
    thread->socket = client;
    HTTP_SERVER_THREAD_START(&(thread->id), http_server_serve_function, thread);
    thread->used = true;
    return;
  }
  fprintf(stderr, "[WARNING]: Could not find a thread for a client\n");
}

void http_server_join_serve_threads(HttpServer *server) {
  for(size_t i=0;i<server->threads_count;i++) {
    HttpServerThread *thread = &(server->threads[i]);
    if(thread->used == false) continue;
    http_close_socket(thread->socket);
    HTTP_SERVER_THREAD_JOIN(thread->id);
    thread->used = false;
    string_buffer_free(&(thread->buffer));
  }
}

void *http_server_listen_function(void *arg) {
  HttpServer *server = (HttpServer *) arg;
  bool *stop = &(server->listen_thread_stop);

  while(!(*stop)) {
    int client;
    HttpAccept accept = http_accept(server->socket, &client);
    if(accept == HTTP_ACCEPT_AGAIN) {
      http_sleep_ms(100);
    } else if(accept == HTTP_ACCEPT_ERROR) {
      fprintf(stderr, "[WARNING]: Failed to accept client\n");
    } else {
      http_server_create_serve_thread(server, client);
    }
  }
  
  return NULL;
}

void http_server_listen_and_serve(HttpServer *server, void (*handle_request)(string method, string path, string headers, string body, int client), size_t number_of_threads) {
  if(!server) return;
  if(number_of_threads == 0) return;

  //ALLOCATE THREADS
  server->threads_count = number_of_threads;
  server->threads = (HttpServerThread *) malloc(server->threads_count*sizeof(HttpServerThread));
  if(!server->threads) {
    fprintf(stderr, "ERROR: Can not allocate enough memory for threads: http_server_listen_and_server\n");
    exit(1);
  }

  for(size_t i=0;i<number_of_threads;i++) {
    HttpServerThread *thread = &server->threads[i];
    thread->used = false;
    thread->buffer = (String_Buffer) {0};
    thread->handle_request = handle_request;
  }

  //START LISTEN THREAD
  if(listen(server->socket, 3) < 0) {
    fprintf(stderr, "ERROR: Can not listen to port: http_server_listen_and_server\n");
    exit(1);    
  }

  server->listen_thread_stop = false;
  HTTP_SERVER_THREAD_START(&server->listen_thread_id, http_server_listen_function, server);
}

void http_server_stop(HttpServer *server) {
  if(!server) return;

  //JOIN ACCEPT THREADS
  http_server_join_serve_threads(server);

  //STOP LISTEN THREAD
  server->listen_thread_stop = true;
  HTTP_SERVER_THREAD_JOIN(server->listen_thread_id);
  
  //DEALLOCATE THREADS
  if(server->threads_count == 0) return;
  if(server->threads) free(server->threads);
}

void http_server_close(HttpServer *server) {
  if(!server) return;
  
#ifdef _WIN32
  if(server->socket != INVALID_SOCKET) {
    WSACleanup();
    closesocket(server->socket);
    shutdown(server->socket, SD_BOTH);    
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

const HttpResponse HTTP_RESPONSE_NOT_FOUND = {
  .code = 404,
  .message = STRING_STATIC("Not Found"),
  .content_type = STRING_STATIC("text/plain"),
  .body = STRING_STATIC("404 - Not Found")
};

const string CRLF = STRING_STATIC("\r\n");

#define HTTP_SEND_STRING(socket, conn, s) do{			\
    if(!http_send_len(socket, conn, (s).data, (s).len)) {	\
      return false;						\
    }								\
  }while(0)							\

#define HTTP_SEND_CRLF(socket, conn) HTTP_SEND_STRING(socket, conn, CRLF)

bool http_response_send(int socket, SSL *conn, HttpResponse response) {
  //Default http_response is zero initialized
  if(response.code == 0) {
    return false;
  }

  int request_len;

char request[MAX_REQUEST_LEN];
 if(response.content_type.len > 0 && response.body.len >0 ) {
   request_len = snprintf(request, MAX_REQUEST_LEN,
			  "HTTP/1.1 %d %.*s\r\n"
			  "Connection: keep-alive\r\n"
			  "Content-Type: %.*s\r\n"
			  "Content-Length: %d\r\n"
			  "\r\n"
			  "%.*s\n",
			  response.code,
			  String_Arg(response.message),
			  String_Arg(response.content_type),
			  (int) response.body.len,
			  String_Arg(response.body));
 }
 else {
   request_len = snprintf(request, MAX_REQUEST_LEN,
			  "HTTP/1.1 %d %.*s\r\n"
			  "Connection: keep-alive\r\n"
			  "\r\n",
			  response.code,
			  String_Arg(response.message));   
 }

 //printf("%s\n", request);

 if(request_len >= MAX_REQUEST_LEN) {
   return false;
 }
  
 return http_send_len(socket, conn, request, request_len);
}

static const string HTTP_CONTENT_TYPE_PLAIN = STRING_STATIC("text/plain");
static const string HTTP_CONTENT_TYPE_HTML = STRING_STATIC("text/html");
static const string HTTP_CONTENT_TYPE_JS = STRING_STATIC("application/javascript");

string http_content_type_for_filename_len(const char *file_path, size_t file_path_len) {
  string content_type = HTTP_CONTENT_TYPE_PLAIN;
  
  size_t i = file_path_len - 1;
  while(i>0 && file_path[i]!='.') i--;

  string ext = string_from(file_path + i, file_path_len - i);
  if(string_eq(ext, STRING(".html"))) {
    content_type = HTTP_CONTENT_TYPE_HTML;
  } else if(string_eq(ext, STRING(".js"))) {
    content_type = HTTP_CONTENT_TYPE_JS;
  } else {
    fprintf(stderr, "[WARNING] Unknown extension: "String_Fmt"\n", String_Arg(ext));
  }
  
  return content_type;
}

void http_server_simple_file_handler(string method, string path, string headers, string body, int client) {
  (void) headers;
  (void) body;
  if(!string_eq(method, STRING("GET"))) {
    if(!http_response_send(client, NULL, HTTP_RESPONSE_NOT_FOUND)) {
      fprintf(stderr, "[WARNING] Failed to write to client\n");
    }
    return;
  }

  bool is_default = string_eq(path, STRING("/"));
  bool allocated_memory = false;

  char buffer[256] = "./index.html";
  char *file_path = buffer;
  if(!is_default && path.len > 256 - 2) {
    file_path = (char *) malloc(sizeof(char) * path.len + 2);
    if(!file_path) {
      panic("Can not allocate enough memory for file_path: http_server_simple_file_handler\n");      
    }
    memcpy(file_path + 1, path.data, path.len);
    file_path[0] = '.';
    file_path[path.len] = 0;
    allocated_memory = true;
  }

  if(!is_default) {
    memcpy(file_path + 1, path.data, path.len);
    file_path[path.len + 1] = 0;
  }

  size_t size;
  char *file_content = slurp_file(file_path, &size);
  if(file_content == NULL) {
    if(!http_response_send(client, NULL, HTTP_RESPONSE_NOT_FOUND)) {
      fprintf(stderr, "[WARNING] Failed to write to client\n");
    }
    return;
  }

  const HttpResponse response = {
    .code=200,
    .message=STRING("OK"),
    .content_type=!is_default
    ? http_content_type_for_filename_len(file_path, path.len+1)
    : HTTP_CONTENT_TYPE_HTML,
    .body=string_from(file_content, size)
  };

  if(!http_response_send(client, NULL, response)) {
    fprintf(stderr, "[WARNING] Failed to write to client\n");
  }
  
  free(file_content);
  if(allocated_memory) free(file_path);
}

//END HTTP_SERVER

#endif //HTTP_IMPLEMENTATION

#endif //HTTP_H
