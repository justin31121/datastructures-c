#ifndef HTTP_H_
#define HTTP_H_

//NOTE: WSACleanup is never called, if called make sure to set http_global_wsa_startup to false
//NOTE: SSLContext is never freed, if freed make sure to set http_global_ssl_ctx to NULL

//MAYBE: Remove some null checks

#include <assert.h>
#include <stdbool.h>
#include <pthread.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/conf.h> //link with -lssl -lcrypto

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
#define HTTP_READ_BUFFER_CAP 8192

#ifdef _WIN32
#include <winsock2.h> //link with -lws2_32
#endif //_WIN32

#ifdef linux
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#endif //linux

typedef struct{
#ifdef _WIN32
  SOCKET socket;
#endif //_WIN32
  
#ifdef linux  
  int socket;
#endif //linux

  SSL* conn;
}Http;

typedef struct{
  string method;
  string route;
  string headers;
  string body;
}HttpRequest;

typedef struct{
  int code;
  string message;
  string body;
  string content_type;
}HttpResponse;

typedef struct{
  Http client;
  bool used;
  String_Buffer buffer;
  pthread_t id;
  void (*handle_request)(const HttpRequest *request, Http *client, void *arg);
  void *arg;
  HttpRequest request;
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
  bool ssl;
}HttpServer;

typedef enum {
  HTTP_ACCEPT_OK = 0,
  HTTP_ACCEPT_AGAIN,
  HTTP_ACCEPT_ERROR
}HttpAccept;

//----------BEGIN HTTP----------
bool http_init(Http *http);

int http_find_hostname(const char *url, size_t url_len, size_t *hostname_len, bool *ssl);
bool http_sleep_ms(int ms);

// -- HTTP1 API
bool http_request(Http *http, const char *url, const char *method, const char* body, const char *content_type, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);
bool http_get(Http *http, const char* url, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);

void http_free(Http *http);
//----------END HTTP----------


//----------BEGIN HTTP_SERVER----------
static const string HTTP_CONTENT_TYPE_PLAIN = STRING_STATIC("text/plain");
static const string HTTP_CONTENT_TYPE_HTML = STRING_STATIC("text/html");
static const string HTTP_CONTENT_TYPE_JS = STRING_STATIC("application/javascript");
static const string HTTP_CONTENT_TYPE_JSON = STRING_STATIC("application/json");

bool http_server_init(HttpServer *server, size_t port, const char *cert_file, const char *key_file);
bool http_server_listen_and_serve(HttpServer *server, void (*handle_request)(const HttpRequest *request, Http *client, void *arg), size_t number_of_threads, void *args, size_t arg_size_bytes);
void http_server_stop(HttpServer *server);
void http_server_free(HttpServer *server);

void *http_server_listen_function(void *arg);
bool http_server_create_serve_thread(HttpServer *server, Http client);
void *http_server_serve_function(void *arg);

// -- UTIL
bool http_server_fill_http_request(string request_string, HttpRequest *request);
bool http_send_http_response(Http *http, const HttpResponse *response, char* buffer, size_t buffer_len);
string http_server_content_type_from_name(string file_name);
void http_server_simple_file_handler(const HttpRequest *request, Http *client, void *arg);
bool http_format_http_response(const HttpResponse *response, char *buffer, size_t buffer_len, size_t *response_len);
//----------END HTTP_SERVER----------


//----------BEGIN SOCKET----------
int http_open();
bool http_bind(int socket, int port);
void http_shutdown(int socket);
void http_close(int socket);

bool http_valid(int socket);
void http_make_nonblocking(int socket);

HttpAccept http_accept(int server, int *client);
HttpAccept http_select(int client, fd_set *read_fds, struct timeval *timeout);
bool http_connect(int socket, bool ssl, const char *hostname, size_t hostname_len);
bool http_send_len(int socket, SSL *conn, const char *buffer, size_t buffer_len);
bool http_read(int socket, SSL *conn, size_t (*Http_Write_Callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);
bool http_read_body(int socket, SSL *conn, size_t (*Http_Write_Callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);

void http_init_external_libs(const char *cert_file, const char *key_file);
//----------END SOCKET----------

size_t _fwrite(const void *data, size_t size, size_t memb, void *userdata);

#ifdef HTTP_IMPLEMENTATION


static SSL_CTX *http_global_ssl_client_ctx = NULL;
static SSL_CTX *http_global_ssl_server_ctx = NULL;
static bool http_global_wsa_startup = false;

//----------BEGIN HTTP----------

bool http_init(Http *http) {
  if(http == NULL) {
    return false;
  }

  http_init_external_libs(NULL, NULL);

  http->socket = http_open();
  if(!http_valid(http->socket)) {
    return false;
  }

  http->conn = SSL_new(http_global_ssl_client_ctx);
  if(!http->conn) {
    return false;
  }

  SSL_set_fd(http->conn, http->socket);

  return true;
}

int http_find_hostname(const char *url, size_t url_len,
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

  int end = cstr_index_of(url + start, url_len - start + 1, "/", 1);
  if(end<0) end = url_len;
  else end += start;

  if(hostname_len) *hostname_len = end - start;
  return start;
}

bool http_sleep_ms(int ms) {
  //TOOD: proper sleep_time if ms is longer than a second
  struct timespec sleep_time;
  if(ms < 1000) {
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = ms * 1000000;    
  } else {
    sleep_time.tv_sec = ms / 1000;
    sleep_time.tv_nsec = 0;
  }
  if(nanosleep(&sleep_time, NULL) == -1) {
    return false;
  }
  return true;
}

bool http_request(Http *http, const char *url, const char *method,
		  const char* body, const char *content_type,
		  size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata),
		  void *userdata)  
{
  Http fallbackHttp;

  bool fallbackIsUsed = http == NULL;
  
  //IF NULL INIT 
  if(fallbackIsUsed) {    
    if(!http_init(&fallbackHttp)) {
      return false;
    }
    http = &fallbackHttp;
  }

  //GET HOSTNAME
  size_t hostname_len;
  size_t url_len = strlen(url);
  bool ssl;
  int hostname = http_find_hostname(url, url_len, &hostname_len, &ssl);

  if(hostname < 0) {
    return false;
  }
  int directory_len = url_len - hostname - hostname_len;

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
    return false;
  }

  //CONNECT
  if(!http_connect(http->socket, ssl, url + hostname, hostname_len)) {
    return false;
  }

  //SSL CONNECT
  if(ssl && SSL_connect(http->conn)!=1) {
    return false;
  }

  //SEND
  if(!http_send_len(http->socket, http->conn, request, request_len)) {
    return false;
  }
  
  //READ
  if(!http_read_body(http->socket, http->conn, write_callback, userdata)) {
    return false;
  }

  if(fallbackIsUsed && http_valid(fallbackHttp.socket)) {
    http_free(&fallbackHttp);    
  }
  
  return true;
}

bool http_get(Http *http, const char* url, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata) {
  return http_request(http, url, "GET", NULL, NULL, write_callback, userdata);
}

void http_free(Http *http) {
  if(http_valid(http->socket)) {
    http_close(http->socket);
  }
  
  if(http->conn) {
    SSL_free(http->conn);
  }
}

//----------END HTTP----------


//----------BEGIN HTTP_SERVER----------
bool http_server_init(HttpServer *server, size_t port, const char *cert_file, const char *key_file) {

  if(server == NULL) {
    return false;
  }

  server->threads = NULL;
  server->threads_count = 0;
  server->ssl = cert_file != NULL && key_file != NULL;

  http_init_external_libs(cert_file, key_file);

  server->socket = http_open();
  if(!http_valid(server->socket)) {
    return false;
  }

  if(!http_bind(server->socket, port)) {
    return false;
  }
  
  http_make_nonblocking(server->socket);
  
  return true;
}

bool http_server_listen_and_serve(HttpServer *server, void (*handle_request)(const HttpRequest *, Http *, void *), size_t number_of_threads, void *_args, size_t arg_size_bytes) {
  if(server == NULL) {
    return false;
  }

  if(number_of_threads == 0) {
    return false;
  }

  char *args = (char *) _args;

  bool args_used = args != NULL;
  if(args_used && arg_size_bytes == 0) {
    return false;
  }

  //ALLOCATE THREADS
  server->threads_count = number_of_threads;
  server->threads = (HttpServerThread *) malloc(server->threads_count*sizeof(HttpServerThread));
  if(!server->threads) {
    return false;
  }

  //INITIALIZE THREADS
  for(size_t i=0;i<number_of_threads;i++) {
    HttpServerThread *thread = &server->threads[i];
    thread->used = false;
    thread->buffer = (String_Buffer) {0};
    thread->handle_request = handle_request;
    thread->arg = args_used ? (args + arg_size_bytes*i) : NULL;
  }

  //START LISTEN THREAD
  if(listen(server->socket, 3) < 0) {
    free(server->threads);
    return false;
  }

  server->threads[0].used = true;
  if(pthread_create(&server->threads[0].id, NULL, http_server_listen_function, server) != 0)  {
    free(server->threads);
    return false;
  }

  return true;
}

void http_server_stop(HttpServer *server) {
  if(server == NULL) {
    return;
  }

  //JOIN ACCEPT THREAD
  for(size_t i=1;i<server->threads_count;i++) {
    HttpServerThread *thread = &(server->threads[i]);
    if(thread->used == false) {
      continue;
    }
    http_free(&thread->client);
    pthread_join(thread->id, NULL);
    thread->used = false;
    string_buffer_free(&(thread->buffer));
  }

  //STOP LISTEN THREAD
  server->threads[0].used = false;
  pthread_join(server->threads[0].id, NULL);

  //DEALLOCATE THREADS
  if(server->threads_count == 0) {
    return;
  }
  if(server->threads) {
    free(server->threads);
  }
}

void http_server_free(HttpServer *server) {
  if(server == NULL) {
    return;
  }

  if(!http_valid(server->socket)) {
    return;
  }

  if(server->threads[0].used == true) {
    http_server_stop(server);
  }

  http_close(server->socket);
  http_shutdown(server->socket);
}

void *http_server_listen_function(void *arg) {
  HttpServer *server = (HttpServer *) arg;
  bool *running = &(server->threads[0].used);

  while(*running) {
    int client;
    HttpAccept accept = http_accept(server->socket, &client);
    if(accept == HTTP_ACCEPT_AGAIN) {
      http_sleep_ms(50);
    } else if(accept == HTTP_ACCEPT_ERROR) {
      //WARN
    } else {
      Http http = {0};
      http.socket = client;
      if(server->ssl) {
	http.conn = SSL_new(http_global_ssl_server_ctx);
	if(!http.conn) {
	  fprintf(stderr, "WARNING: Https failed to generate SSL_connection\n");
	  continue;
	}
	
	//WTF
#ifdef _WIN32	
	unsigned long mode = 0;
	ioctlsocket(client, FIONBIO, &mode);
#endif //_WIN32
 
	SSL_set_fd(http.conn, client);
	int accept_res = SSL_accept(http.conn);
	if(accept_res <= 0) {
	  ERR_print_errors_fp(stderr);
	  http_free(&http);
	  fprintf(stderr, "WARNING: Https client failed to connect\n");
	  continue;
	}
      }
      if(!http_server_create_serve_thread(server, http)) {
	fprintf(stderr, "WARNING: No server thread is avaible\n");
	//TODO: Maybe serve client once on listen thread
      }
    }
  }

  return NULL;
}

bool http_server_create_serve_thread(HttpServer *server, Http client) {  
  for(size_t i=1;i<server->threads_count;i++) {
    HttpServerThread *thread = &(server->threads[i]);
    if(thread->used==true) continue;
    thread->client = client;
    if(pthread_create(&(thread->id), NULL, http_server_serve_function, thread) != 0) {
      return false;
    }
    thread->used = true;
    return true;
  }
  
  return false;
}

void *http_server_serve_function(void *_arg) {
  HttpServerThread *thread = (HttpServerThread *) _arg;
  Http client = thread->client;
  bool *used = &(thread->used);
  String_Buffer *buffer = &(thread->buffer);
  void (*handle_request)(const HttpRequest *, Http *, void *) = thread->handle_request;
  HttpRequest *request = &(thread->request);
  *request = (HttpRequest) {0};
  void* arg = thread->arg;

  http_make_nonblocking(client.socket);

  fd_set read_fds = {0};
  FD_SET(client.socket, &read_fds);
  
  struct timeval timeout;
  timeout.tv_sec = 2;
  timeout.tv_usec = 0;

  while(true) {
    buffer->len = 0;
    HttpAccept select = http_select(client.socket, &read_fds, &timeout); 
    if(select != HTTP_ACCEPT_OK) {
      break;
    }

    if(!http_read(client.socket, client.conn, string_buffer_callback, buffer)) {
      break;
    }

    if(handle_request != NULL) {
      if(!http_server_fill_http_request(string_from(buffer->data, buffer->len), request)) {
	//WARN
      }      
      handle_request(request, &client, arg);
    }
  }

  http_free(&client);
  *used = false;

  return NULL;
}

bool http_server_fill_http_request(string request_string, HttpRequest *request) {
  if(request == NULL) {
    return false;
  }

  string request_string_origin = request_string;
  
  size_t headersStart = 0;
  size_t bodyStart = 0;

  bool methodFound = 0;
  bool headerFound = 0;  
  while(request_string.len) {
    string line = string_trim(string_chop_by_delim(&request_string, '\n'));

    if(!methodFound) {
      request->method = string_trim(string_chop_by_delim(&line, ' '));
      request->route = string_trim(string_chop_by_delim(&line, ' '));
      methodFound = true;
      continue;
    }

    if(headersStart == 0) {
      headersStart = line.data - request_string_origin.data;
    }

    if(!headerFound) {
      if(line.len == 0) headerFound = true;
      continue;
    }

    if(bodyStart == 0) {
      bodyStart = line.data - request_string_origin.data;
    }
    break;
  }
  
  request->body = string_from(request_string_origin.data + bodyStart,
			      request_string_origin.len - bodyStart);
  request->headers = string_trim(string_from(request_string_origin.data + headersStart,
					     request_string_origin.len - headersStart - request->body.len));  
  return true;
}

bool http_send_http_response(Http *http, const HttpResponse *response, char* buffer, size_t buffer_len) {
  if(!http) {
    return false;
  }

  if(response == NULL) {    
    const char *not_found_string =
      "HTTP/1.1 404 Not Found\r\n"
      "Content-Type: text/plain\r\n"
      "Content-Length: 15\r\n"
      "\r\n"
      "404 - Not Found";
    return http_send_len(http->socket, http->conn, not_found_string, strlen(not_found_string));
  }

  if(buffer == NULL) {
    char stack_buffer[MAX_REQUEST_LEN];
    buffer = stack_buffer;
    buffer_len = MAX_REQUEST_LEN;
  }

  size_t response_len;
  if(!http_format_http_response(response, buffer, buffer_len, &response_len)) {
    return false;
  }
  
  return http_send_len(http->socket, http->conn, buffer, response_len);
}

string http_server_content_type_from_name(string file_name) {
  string content_type = HTTP_CONTENT_TYPE_PLAIN;

  if(file_name.len <= 2) {
    return content_type;
  }

  int i = (int) file_name.len - 1;
  while(i>0 && file_name.data[i] != '.') i--;
  
  string ext = string_from(file_name.data + i, file_name.len - i);
  if(string_eq(ext, STRING(".html"))) {
    content_type = HTTP_CONTENT_TYPE_HTML;
  } else if(string_eq(ext, STRING(".js"))) {
    content_type = HTTP_CONTENT_TYPE_JS;
  }else if(string_eq(ext, STRING(".json"))) {
    content_type = HTTP_CONTENT_TYPE_JSON;
  } else {
    fprintf(stderr, "[WARNING] Unknown extension: "String_Fmt"\n", String_Arg(ext));
  }
  
  return content_type;
}

//TODO: either make it variable by a buffer or remove it, since it needs to allocate memroy for the response
void http_server_simple_file_handler(const HttpRequest *request, Http *client, void *arg) {
  (void) arg;

  if(!string_eq(request->method, STRING("GET"))) {
    http_send_http_response(client, NULL, NULL, 0);
    return;
  }
  
  //GET FILE_PATH
  char file_path[256] = "./index.html";
  string path = request->route;
  assert(path.len < 256 - 1);
  
  if(!string_eq(request->route, STRING("/"))) {
    memcpy(file_path + 1, path.data, path.len);
    file_path[path.len + 1] = 0;
  }

  //SLURP_FILE
  size_t size;
  char *content = slurp_file(file_path, &size);
  if(!content) {
    http_send_http_response(client, NULL, NULL, 0);
    return;
  }
  
  HttpResponse response = {0};
  response.code = 200;
  response.message = STRING("OK");
    
  response.body = string_from(content, size);
  response.content_type = http_server_content_type_from_name(string_from_cstr(file_path));

  http_send_http_response(client, &response, NULL, 0);
  free(content);
}

bool http_format_http_response(const HttpResponse *response, char *buffer, size_t buffer_len, size_t *response_len) {
  if(response == NULL) {
    return false;
  }

  size_t len;

  if(response->content_type.len > 0 && response->body.len >0 ) {
    len = snprintf(buffer, buffer_len,
			  "HTTP/1.1 %d %.*s\r\n"
			  "Connection: keep-alive\r\n"
			  "Content-Type: %.*s\r\n"
			  "Content-Length: %d\r\n"
			  "\r\n"
			  "%.*s\n",
			  response->code,
			  String_Arg(response->message),
			  String_Arg(response->content_type),
			  (int) response->body.len,
			  String_Arg(response->body));
  }
  else {
    len = snprintf(buffer, buffer_len,
			  "HTTP/1.1 %d %.*s\r\n"
			  "Connection: keep-alive\r\n"
			  "\r\n",
			  response->code,
			  String_Arg(response->message));   
  }

  if(len >= buffer_len || len==0) {
    return false;
  }

  if(response_len) *response_len = len;
  return true;
}
//----------END HTTP_SERVER----------


//----------BEGIN SOCKET----------
int http_open() {
#ifdef _WIN32
  return WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
#endif //_WIN32
#ifdef linux
  struct protoent *protoent = getprotobyname("tcp");
  if(!protoent) {
    return -1;
  }
  
  return socket(AF_INET, SOCK_STREAM, /*protoent->p_proto*/ 0);
#endif //linux
}

bool http_bind(int socket, int port) {
  if(!http_valid(socket)) {
    return false;
  }

#ifdef _WIN32
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if(bind(socket, (SOCKADDR*) &addr, sizeof(addr)) == SOCKET_ERROR) {
    closesocket(socket);
    return false;
  }
  
#endif //_WIN32

#ifdef linux
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if(bind(socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    close(socket);
    return false;
  }
#endif //linux

  return true;
}

void http_shutdown(int socket) {
#ifdef _WIN32
  shutdown(socket, SD_BOTH);
#endif //_WIN32
#ifdef linux
  shutdown(socket, SHUT_RDWR);
#endif //linux
}

void http_close(int socket) {
#ifdef _WIN32
  closesocket(socket);
#endif //_WIN32
#ifdef linux
  close(socket);
#endif //linux
}


bool http_valid(int socket) {
#ifdef _WIN32
  return ((SOCKET) socket) != INVALID_SOCKET;
#endif //_WIN32
#ifdef linux
  return socket !=-1;
#endif //linux
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

HttpAccept http_accept(int socket, int *out_client) {
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

  if(out_client) *out_client = client;
  return HTTP_ACCEPT_OK;
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

bool http_connect(int socket, bool ssl, const char *hostname, size_t hostname_len) {
  if(!http_valid(socket)) {
    return false;
  }

  if(hostname == NULL) {
    return false;
  }

#ifdef _WIN32
  SOCKADDR_IN addr = {0};
#endif //WIN32
#ifdef linux
  struct sockaddr_in addr = {0};
#endif //linux

  //HOSTNAME to Cstr
  char name[hostname_len+1];
  memcpy(name, hostname, hostname_len);
  name[hostname_len] = 0;

  struct hostent *hostent = gethostbyname(name);
  if(!hostent) {
    return false;
  }
  
#ifdef _WIN32
  addr.sin_addr.s_addr = *((unsigned long*) hostent->h_addr);
#endif //WIN32
#ifdef linux
  in_addr_t in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
  if(in_addr == (in_addr_t) -1) {
    return false;
  }
  addr.sin_addr.s_addr = in_addr;
#endif //linux
  
  addr.sin_family = AF_INET;
  addr.sin_port = htons(ssl ? HTTPS_PORT : HTTP_PORT);
  if(connect(socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    
    return false;
  }
  
  return true; 
}

bool http_send_len(int socket, SSL *conn, const char *buffer, size_t _buffer_size) {
  if(!http_valid(socket)) {
    return false;
  }

  if(buffer == NULL) {
    return false;
  }

  ssize_t buffer_size = (ssize_t) _buffer_size;
  
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

bool http_read(int socket, SSL *conn, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata) {
  if(!http_valid(socket)) {
    return false;
  }
  
  char buffer[HTTP_READ_BUFFER_CAP];
  
  ssize_t nbytes_total = 0;
  ssize_t nbytes_last;
  do {
    if(conn != NULL) {
      nbytes_last = SSL_read(conn, buffer, HTTP_READ_BUFFER_CAP);
    }
    else {
      nbytes_last = recv(socket, buffer, HTTP_READ_BUFFER_CAP, 0);
    }

#ifdef linux
    if(nbytes_last < 0) {
      if(errno != EAGAIN && errno !=EWOULDBLOCK) {
	return false;
      }
      return nbytes_total > 0;
    }
#endif //linux
#ifdef _WIN32
    if(nbytes_last == SOCKET_ERROR) {
      if(WSAGetLastError() != WSAEWOULDBLOCK) {
	return false;
      }
      return nbytes_total!=0 && nbytes_total!=SOCKET_ERROR;
    }
#endif //_WIN32

    nbytes_total += nbytes_last;
    
    if(nbytes_last>0 && write_callback != NULL && write_callback) {
      (*write_callback)(buffer, nbytes_last, 1, userdata);	
    }
    
  }while(true);

  return false;
}

bool http_read_body(int socket, SSL *conn, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata) {
  if(!http_valid(socket)) {
    return false;
  }

  int inHeader = -1;
  char window[4] = {0};
  bool first = true;
  int s = 0;
  
#if 0
  inHeader = 0;
#endif 

  char buffer[HTTP_READ_BUFFER_CAP];

  ssize_t nbytes_total;
  do{
    if(conn != NULL) {
      nbytes_total = SSL_read(conn, buffer, HTTP_READ_BUFFER_CAP);
    }
    else {
      nbytes_total = recv(socket, buffer, HTTP_READ_BUFFER_CAP, 0);
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
      
      for(int i=0;i<HTTP_READ_BUFFER_CAP;i++) {
	if(window[s]=='\r' && window[(s+1)%4]=='\n' && window[(s+2)%4]=='\r' && window[(s+3)%4]=='\n') {
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
  
  return true;
}

void http_init_external_libs(const char *cert_file, const char *key_file) {  
  if(!http_global_ssl_client_ctx) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    http_global_ssl_client_ctx = SSL_CTX_new(TLS_client_method());
    http_global_ssl_server_ctx = SSL_CTX_new(TLS_server_method());

    if(!http_global_ssl_client_ctx) {
      ERR_print_errors_fp(stderr);
      panic("Can not initialize libopenssl");
    }

    if(!http_global_ssl_server_ctx) {
      ERR_print_errors_fp(stderr);
      panic("Can not initialize libopenssl");
    }    
  }
  
  if (cert_file != NULL && SSL_CTX_use_certificate_file(http_global_ssl_server_ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    panic("Can not initialize libopenssl: cert_file");
  }

  if (key_file != NULL && SSL_CTX_use_PrivateKey_file(http_global_ssl_server_ctx, key_file, SSL_FILETYPE_PEM) <= 0 ) {
    ERR_print_errors_fp(stderr);
    panic("Can not initialize libopenssl: key_file");
  }

  if (cert_file != NULL && key_file != NULL && !SSL_CTX_check_private_key(http_global_ssl_server_ctx)) {
    panic("Private key does not match the public certificate\n");
  }
  
#ifdef _WIN32
  if(!http_global_wsa_startup) {
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
      fprintf(stderr, "ERROR: Can not initialize libws2\n");
      exit(1);
    }
    
    
    http_global_wsa_startup = true;
  }
#endif //_WIN32
#ifdef linux
  (void) http_global_wsa_startup;
#endif //linux
}
//----------END SOCKET----------

size_t _fwrite(const void *data, size_t size, size_t memb, void *userdata) {
  return fwrite(data, size, memb, (FILE *) userdata);
}

#endif //HTTP_IMPLEMENTATION

#endif //HTTP_H
