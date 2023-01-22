#ifndef HTTP_H_
#define HTTP_H_

//NOTE: WSACleanup is never called, if called make sure to set http_global_wsa_startup to false
//NOTE: SSLContext is never freed, if freed make sure to set http_global_ssl_ctx to NULL

//MAYBE: Remove some null checks

#include <assert.h>
#include <stdbool.h>
#include <pthread.h>

#ifndef HTTP_NO_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/conf.h> //link with -lssl -lcrypto
#endif //HTTP_NO_SSL

#ifdef HTTP_IMPLEMENTATION

#ifndef UTIL_IMPLEMENTATION
#define UTIL_IMPLEMENTATION
#endif //UTIL_IMPLEMENTATION

#endif //HTTP_IMPLEMENTATION

#include "./util.h"

#ifdef HTTP_IMPLEMENTATION

#ifndef STRING_IMPLEMENTATION
#define STRING_IMPLEMENTATION
#endif //STRING_IMPLEMENTATION

#endif //HTTP_IMPLEMENTATION
#include "./string.h"

#define HTTP_PORT 80
#define HTTPS_PORT 443
#define HTTP_BUFFER_CAP 8192

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
#ifndef HTTP_NO_SSL
  SSL* conn;
#endif //HTTP_NO_SSL
  const char *host;
}Http;

typedef struct{
  string method;
  string route;
  string headers;
  string body;
}HttpRequest;

typedef enum{
  HTTP_STATUS_OK = 0,
  HTTP_STATUS_INTERNAL_ERROR,
  COUNT_HTTP_STATUS
}HttpStatus;

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
bool http_init(Http *http, const char *hostname, bool ssl);

int http_find_hostname(const char *url, size_t url_len, size_t *hostname_len, bool *ssl);
bool http_sleep_ms(int ms);

bool http_read_body(Http *http, size_t (*Http_Write_Callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);

// -- HTTP1 API
bool http_request(Http *http, const char *route, const char *method, const char* body, const char *content_type, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);
bool http_get(const char *url, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);
void http_free(Http *http);
//----------END HTTP----------


//----------BEGIN HTTP_SERVER----------
static const char* HTTP_CONTENT_TYPE_PLAIN = "text/plain";
static const char* HTTP_CONTENT_TYPE_HTML = "text/html";
static const char* HTTP_CONTENT_TYPE_JS = "application/javascript";
static const char* HTTP_CONTENT_TYPE_JSON = "application/json";

bool http_server_init(HttpServer *server, size_t port, const char *cert_file, const char *key_file);
bool http_server_listen_and_serve(HttpServer *server, void (*handle_request)(const HttpRequest *request, Http *client, void *arg), size_t number_of_threads, void *args, size_t arg_size_bytes);
void http_server_stop(HttpServer *server);
void http_server_free(HttpServer *server);

void *http_server_listen_function(void *arg);
bool http_server_create_serve_thread(HttpServer *server, Http client);
void *http_server_serve_function(void *arg);

bool http_server_fill_http_request(string request_string, HttpRequest *request);

// -- UTIL
bool http_send_not_found(Http *http);

const char* http_server_content_type_from_name(string file_name);
void http_send_files(Http *client, const char *dir, const char *home, string file_path);

bool http_open_file(FILE **f, const char* file_path);
bool http_open_file2(FILE **f, string file_path);
bool http_open_file3(FILE **f, const char *dir, string file_path);
bool http_send_file(FILE **f, Http *http, const char *add_headers);
void http_close_file(FILE **f);

void http_server_simple_file_handler(const HttpRequest *request, Http *client, void *arg);
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
bool http_connect(int socket, bool ssl, const char *hostname);
bool http_respond(Http *http, HttpStatus status, const char *content_type, const void *body, size_t body_size_bytes);
bool http_sendf(Http *http, const char *fmt, ...);
bool http_send_len(const char *buffer, size_t buffer_len, Http *http);
bool http_send_len2(const char *buffer, size_t buffer_len, void *http);
bool http_read(Http *http, size_t (*Http_Write_Callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);


void http_init_external_libs(const char *cert_file, const char *key_file);
//----------END SOCKET----------

size_t _fwrite(const void *data, size_t size, size_t memb, void *userdata);

#ifdef HTTP_IMPLEMENTATION

#ifndef HTTP_NO_SSL
static SSL_CTX *http_global_ssl_client_ctx = NULL;
static SSL_CTX *http_global_ssl_server_ctx = NULL;
#endif //HTTP_NO_SSL
static bool http_global_wsa_startup = false;

//----------BEGIN HTTP----------

bool http_init(Http *http, const char *hostname, bool ssl) {
  if(http == NULL) {
    return false;
  }
  if(hostname == NULL) {
    return false;
  }

  http_init_external_libs(NULL, NULL);

  http->socket = http_open();
  if(!http_valid(http->socket)) {
    return false;
  }


#ifndef HTTP_NO_SSL
  if(ssl) {
    http->conn = SSL_new(http_global_ssl_client_ctx);
    if(!http->conn) {
      return false;
    }
    SSL_set_fd(http->conn, http->socket);
  } else {
    http->conn = NULL;
  }
#else
  if(ssl) {
    warn("Compile with openssl and remove HTTP_NO_SSL, to request the url");
    return false;
  }
#endif //HTTP_NO_SSL

  if(!http_connect(http->socket, ssl, hostname)) {
    warn("connect failed");
    return false;
  }

#ifndef HTTP_NO_SSL
  if(ssl && SSL_connect(http->conn)!=1) {
    return false;
  }
#endif //HTTP_NO_SSL

  http->host = hostname;

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

bool http_request(Http *http, const char *route, const char *method,
		  const char* body, const char *content_type,
		  size_t (*write_callback)(const void*, size_t, size_t, void *),
		  void *userdata)
{
  not_null(http);

  bool hasBody = body != NULL && content_type != NULL;
  
  //SEND
  char request_buffer[HTTP_BUFFER_CAP];
  if(!hasBody) {
    if(!sendf(http_send_len2, http, request_buffer, HTTP_BUFFER_CAP,
	      "%s %s HTTP/1.1\r\n"
	      "Host: %s\r\n"
	      "\r\n", method, route, http->host)) {
      warn("send failed");
      return false;
    }
  } else {
    if(!sendf(http_send_len2, http, request_buffer, HTTP_BUFFER_CAP,
	      "%s %s HTTP/1.1\r\n"
	      "Host: %s\r\n"
	      "Content-Type: %s\r\n"
	      "Content-Length: %d\r\n"
	      "\r\n"
	      "%s", method, route, http->host, content_type, strlen(body), body)) {
      warn("send failed");
      return false;
    }    
  }
  
  //READ
  if(!http_read_body(http, write_callback, userdata)) {
    warn("read failed");
    return false;
  }
  
  return true;
}

bool http_get(const char *url, size_t (*write_callback)(const void *, size_t,size_t, void *), void *userdata) {
  size_t url_len = strlen(url);
  bool ssl;
  size_t hostname_len;
  
  int hostname = http_find_hostname(url, url_len, &hostname_len, &ssl);
  if(hostname < 0) {
    warn("can not find hostname");
    return false;
  }
  
  int directory_len = url_len - hostname - hostname_len;
  const char *route = "/";
  if(directory_len>0) {
    route = url + hostname + hostname_len;
  }

  char name[hostname_len+1];
  memcpy(name, url + hostname, hostname_len);
  name[hostname_len] = 0;

  Http http;
  if(!http_init(&http, name, ssl)) {
    warn("http_init failed");
    return false;
  }

  if(!http_request(&http, route, "GET", NULL, NULL, write_callback, userdata)) {
    warn("http_request failed");
    return false;
  }

  http_free(&http);
  
  return true;
}

void http_free(Http *http) {
  if(http_valid(http->socket)) {
    http_close(http->socket);
  }

#ifndef HTTP_NO_SSL
  if(http->conn) {
    SSL_free(http->conn);
  }
#endif //HTTP_NO_SSL
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
#ifndef HTTP_NO_SSL
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
#endif //HTTP_NO_SSL
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

    if(!http_read(&client, string_buffer_callback, buffer)) {
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

bool http_send_not_found(Http *http) {
  if(!http_valid(http->socket)) {
    return false;
  }

  static const char *not_found_string =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 15\r\n"
    "\r\n"
    "404 - Not Found";
  return http_send_len(not_found_string, strlen(not_found_string), http); 
}

const char* http_server_content_type_from_name(string file_name) {
  const char *content_type = HTTP_CONTENT_TYPE_PLAIN;

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

void http_send_files(Http *client, const char *dir, const char *home, string file_path) {
  if(string_eq(file_path, STRING("/"))) {
    file_path = string_from_cstr(home);
  }
  
  FILE *f;
  if(!http_open_file3(&f, dir, file_path)) {
    http_send_not_found(client);
    return;
  }
  http_send_file(&f, client, http_server_content_type_from_name(file_path));
  http_close_file(&f);
}

bool http_open_file(FILE **f, const char *file_path) {
  FILE *file = fopen(file_path, "rb");
  if(!file) {
    fprintf(stderr, "[WARNING]: Can not open file '%s' because: %s\n", file_path, strerror(errno));
    return false;
  }
  clearerr(file);

  if(f) *f = file;
  
  return true;
}

bool http_open_file2(FILE **f, string file_path) {
  char buffer[MAX_PATH+1];
  assert(file_path.len < MAX_PATH);
  memcpy(buffer, file_path.data, file_path.len);
  buffer[file_path.len] = 0;
  return http_open_file(f, buffer);
}

bool http_open_file3(FILE **f, const char *dir, string file_path) {
  size_t dir_len = strlen(dir);
  if(dir_len == 0) {
    return false;
  }
  
  char buffer[MAX_PATH+1];
  assert(file_path.len + dir_len < MAX_PATH);
  memcpy(buffer, dir, dir_len);
  memcpy(buffer + dir_len, file_path.data, file_path.len);
  buffer[dir_len + file_path.len] = 0;
  
  return http_open_file(f, buffer);
}

bool http_send_file(FILE **f, Http *http, const char *content_type) {
  if(!http_valid(http->socket)) {
    return false;
  }
  if(!f) {
    return false;
  }
  if(!content_type) {
    return false;
  }

  char buffer[HTTP_BUFFER_CAP];
  size_t buffer_off = 0;
  size_t buffer_cap = HTTP_BUFFER_CAP;

  size_t buffer_size = snprintf(buffer, HTTP_BUFFER_CAP, "%s%s%s\r\n", "HTTP/1.1 200 OK\r\nContent-Type: ",
				content_type,
				"\r\nConnection: keep-alive\r\n"
				"Transfer-Encoding: chunked\r\n");
  
  if(buffer_size >= HTTP_BUFFER_CAP) {
    panic("The Content-Type is too big, to fit into the HTTP_BUFFER_CAP.");
    return false;
  }

  buffer_off = buffer_size;
  buffer_cap = HTTP_BUFFER_CAP - buffer_off;

  static const char *end_carriage = "0\r\n\r\n";
  static const size_t end_carriage_len = 5;
  
  while(true) {
    bool bbreak = false;//0129\r\n ... body ... \r\n
    size_t nbytes = fread(buffer + buffer_off + 6, 1, -6 + buffer_cap - 2, *f);
    if(nbytes != buffer_cap - 2 - 6) {
      if(ferror(*f)) {
	fprintf(stderr, "[WARNING]: Can not read file because: %s\n", strerror(errno));
	return false;
      }
      if(feof(*f)) {
	bbreak = true;
      }
    }
    buffer[buffer_off + 4] = '\r';
    buffer[buffer_off + 5] = '\n';
    buffer[buffer_off + 6 + nbytes] = '\r';
    buffer[buffer_off + 6 + nbytes + 1] = '\n';
   
    if(nbytes > 0) {
      //4 := 0x2000 = HTTP_BUFFER_CAP
      //If HTTP_BUFFER_CAP exceeds 4 digits in hexadecimal, adjust 4 to 5, ...
      //CONVERTING nbytes to hex in buffer[0] - buffer[3]
      size_t n = nbytes;
      int i = 0;
      while(n > 0) {
	size_t m = n % 16;
	buffer[buffer_off + 4 - i++ - 1] = m<10 ? m + '0' : m + 'W';
	n /= 16;
      }
      while(i < 4) buffer[buffer_off + 4 - i++ - 1] = '0';

      if(bbreak && (6 + nbytes + 2 + end_carriage_len < buffer_cap)) {
	memcpy(buffer + buffer_off + 6 + nbytes + 2, end_carriage, end_carriage_len);
	nbytes += end_carriage_len;
      }
#ifdef HTTP_DEBUG
      fwrite(buffer, buffer_off + 6 + nbytes + 2, 1, stdout);
#endif //HTTP_DEBUG
      if(!http_send_len(buffer, buffer_off + 6 + nbytes + 2, http)) {	
	return false;
      }
    }
    if(bbreak) {
      if(6 + nbytes + 2 + 5 >= buffer_cap) {
	if(!http_send_len(end_carriage, end_carriage_len, http)) {	  
	  return false;
	}
      }
      break;
    }
    buffer_cap = HTTP_BUFFER_CAP;
    buffer_off = 0;
  }

  return true;
}

void http_close_file(FILE **f) {
  if(f) fclose(*f);
}

void http_server_simple_file_handler(const HttpRequest *request, Http *client, void *arg) {
  (void) arg;

  if(string_eq(request->method, STRING("GET"))) {
    http_send_files(client, ".", "/index.html", request->route);
    return;
  }

  http_send_not_found(client);
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

bool http_connect(int socket, bool ssl, const char *hostname) {
  if(!http_valid(socket)) {
    warn("invalid socket");
    return false;
  }

  if(hostname == NULL) {
    warn("hostname is null");
    return false;
  }

#ifdef _WIN32
  SOCKADDR_IN addr = {0};
#endif //WIN32
#ifdef linux
  struct sockaddr_in addr = {0};
#endif //linux
  struct hostent *hostent = gethostbyname(hostname);
  if(!hostent) {
    warn("hostent is null");
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
    warn("socket connect failed");
    return false;
  }
  
  return true; 
}

bool http_respond(Http *http, HttpStatus status, const char *content_type, const void *body, size_t body_size_bytes) {
  if(!http_valid(http->socket)) {
    return false;
  }

  if(!content_type) {
    return false;
  }

  if(!body) {
    return false;
  }

  const char *prefix = NULL;
  
  static_assert(COUNT_HTTP_STATUS == 2, "The amount of HttpStatus has changed.");
  switch(status) {
  case HTTP_STATUS_OK:
    prefix = "HTTP/1.1 200 OK";
    break;
  case HTTP_STATUS_INTERNAL_ERROR:
    prefix = "HTTP/1.1 500 Internal Server Error";
    break;
  default:
    panic("unreachable");
  }

  char buffer[HTTP_BUFFER_CAP];
  return sendf(http_send_len2, http, buffer, HTTP_BUFFER_CAP,
	       "%s\r\n"
	       "Connection: keep-alive\r\n"
	       "Content-Type %s\r\n"
	       "Content-Length: %d\r\n"
	       "\r\n"
	       "%.*s",
	       prefix, content_type, body_size_bytes, body_size_bytes, body);
}

#define http_sendf(http, fmt, ...) do{					\
    char buffer[HTTP_BUFFER_CAP];					\
    sendf(http_send_len2, http, buffer, HTTP_BUFFER_CAP, fmt, __VA_ARGS__); \
  }while(0)

bool http_send_len(const char *buffer, size_t _buffer_size, Http *http) {
  if(!http_valid(http->socket)) {
    return false;
  }
  
  if(buffer == NULL) {
    return false;
  }

  ssize_t buffer_size = (ssize_t) _buffer_size;
  
  ssize_t nbytes_last;
  ssize_t nbytes_total = 0;
  while(nbytes_total < buffer_size) {
#ifndef HTTP_NO_SSL
    if(http->conn != NULL) {
      nbytes_last = SSL_write(http->conn, buffer + nbytes_total, buffer_size - nbytes_total);
    } else {
      nbytes_last = send(http->socket, buffer + nbytes_total, buffer_size - nbytes_total, 0);
    }
#else
    nbytes_last = send(http->socket, buffer + nbytes_total, buffer_size - nbytes_total, 0);
#endif //HTTP_NO_SSL
    
    if(nbytes_last == -1) {
      return false;
    }
    nbytes_total += nbytes_last;
  }
  
  return true;
}

bool http_send_len2(const char *buffer, size_t buffer_len, void *http) {
  return http_send_len(buffer, buffer_len, (Http *) http);
}

bool http_read(Http *http, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata) {
  if(!http_valid(http->socket)) {
    return false;
  }
  
  char buffer[HTTP_BUFFER_CAP];
  
  ssize_t nbytes_total = 0;
  ssize_t nbytes_last;
  do {
#ifndef HTTP_NO_SSL
    if(http->conn != NULL) {
      nbytes_last = SSL_read(http->conn, buffer, HTTP_BUFFER_CAP);
    }
    else {
      nbytes_last = recv(http->socket, buffer, HTTP_BUFFER_CAP, 0);
    }
#else
    nbytes_last = recv(http->socket, buffer, HTTP_BUFFER_CAP, 0);
#endif //HTTP_NO_SSL

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

bool http_read_body(Http *http, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata) {
  not_null(http);
  
  if(!http_valid(http->socket)) {
    return false;
  }

  char buffer[HTTP_BUFFER_CAP];
  int64_t content_length = -1;
  int64_t need = -1;
  
  uint64_t read = 0;

  bool body = false;

  ssize_t nbytes_total;
  do{
#ifndef HTTP_NO_SSL
    if(http->conn != NULL) {
      nbytes_total = SSL_read(http->conn, buffer, HTTP_BUFFER_CAP);
    }
    else {
      nbytes_total = recv(http->socket, buffer, HTTP_BUFFER_CAP, 0);
    }    
#else
    nbytes_total = recv(http->socket, buffer, HTTP_BUFFER_CAP, 0);
#endif //HTTP_NO_SSL

    if(nbytes_total == -1) {
      return false;
    }

    size_t offset = 0;
    if(!body) {
      string s = string_from(buffer, nbytes_total);
      while(s.len) {
	string line = string_chop_by_delim(&s, '\n');
	if(line.len && line.data[0]=='\r') {
	  body = true;
	  offset = line.data+2 - buffer;
	  break;
	}
	string key = string_trim(string_chop_by_delim(&line, ':'));
	string value = string_trim(line);
#ifdef HTTP_DEBUG
	printf(String_Fmt": "String_Fmt"\n", String_Arg(key), String_Arg(value));
#endif	
	if(string_eq(key, STRING("Content-Length"))) {
	  if(!string_chop_int64_t(&value, &content_length)) {
	    warn("Failed to parse content length");
	    content_length = -1;
	  }
	}
	else if(string_eq(key, STRING("Transfer-Encoding")) &&
		string_eq(value, STRING("chunked"))) {
	  content_length = -2;
	}
      }
    }

    if(body && write_callback!=NULL) {
      if(content_length > 0) { //CONTENT_LENGTH: 69
	uint64_t len = (uint64_t) (nbytes_total - (ssize_t) offset);
	if(content_length != -1 && read + len > (uint64_t) content_length) {
	  len = content_length - read;
	}
	if(len > 0) {
	  write_callback(buffer + offset, len, 1, userdata);	  
	}
	read += len;

	if(read >= (uint64_t) content_length) {
	  break;
	}
      } else if(content_length == -2) { //TRANSFER_ENCODING: chunked
	string s = string_from(buffer + offset, nbytes_total - offset);
	while(s.len) {	  
	  string line = string_chop_by_delim(&s, '\n');
	  if(!line.len) continue;
	  if(line.data[line.len-1] == '\r') {
	    if(line.data[0] == '\r') break;
	    uint64_t n;
	    if(string_chop_hex(&line, &n) && line.len>0 && line.data[0] == '\r') {
	      need = (int64_t) n;
	      read = 0;
	      continue;
	    }
	    string_chop_right(&line, 1);
	  }	  
	  uint64_t len = line.len;
	  if(read + len > (uint64_t) need) {
	    len = (uint64_t) need - read;
	  }

	  if(len > 0) {
	    write_callback(line.data, len, 1, userdata);
	  }
	  read += len;
	}

	if(need == 0) {
	  break;
	}
      } else {
	warn("Server does not provide a content-length and does not support Chunked Encoding");
      } 
    }


  }while(nbytes_total > 0);
  
  return true;
}

void http_init_external_libs(const char *cert_file, const char *key_file) {
#ifndef HTTP_NO_SSL
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
#else
  (void) cert_file;
  (void) key_file;
#endif //HTTP_NO_SSL
  
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
