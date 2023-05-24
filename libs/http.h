#ifndef HTTP_H_
#define HTTP_H_

//NOTE: WSACleanup is never called, if called make sure to set http_global_wsa_startup to false
//NOTE: SSLContext is never freed, if freed make sure to set http_global_ssl_ctx to NULL

//MAYBE: Remove some null checks

#ifndef HTTP_NO_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>  //link with -lssl -lcrypto
#include <openssl/conf.h> //or        libsslMT.lib libcryptoMT.lib
#endif //HTTP_NO_SSL

#define HTTP_PORT 80
#define HTTPS_PORT 443
#define HTTP_BUFFER_CAP 8192

#ifdef _WIN32
#  include <winsock2.h>
#  include <windows.h>
#  include <ws2tcpip.h>
typedef SSIZE_T ssize_t;
#  include <malloc.h> // _malloca
#endif //_WIN32

#ifdef linux
#  include <pthread.h>
#  include <sys/socket.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <arpa/inet.h>
#  include <netdb.h>
typedef int SOCKET;
#endif //

#ifdef HTTP_IMPLEMENTATION

#define SHA1_IMPLEMENTATION
#define BASE64_IMPLEMENTATION
#define STRING_IMPLEMENTATION
#define THREAD_IMPLEMENTATION
#define IO_IMPLEMENTATION

#endif //HTTP_IMPLEMENTATION

#include <assert.h>
#include <stdbool.h>

#include "./string.h"
#include "./base64.h"
#include "./sha1.h"
#include "./thread.h"
#include "./io.h"

#ifndef HTTP_DEF
#define HTTP_DEF static inline
#endif //HTTP_DEF

typedef struct{
#ifdef _WIN32
  SOCKET socket;
#else
  int socket;
#endif //linux
  const char *host;
  size_t host_len;
  
#ifndef HTTP_NO_SSL
  SSL* conn;
#endif //HTTP_NO_SSL
}Http;

typedef struct {
  char data[HTTP_BUFFER_CAP];
  size_t size;
}HttpHeader;

typedef struct{
  string method;
  string route;
  string headers;
  string body;
}HttpRequest;

typedef enum{
  HTTP_STATUS_OK = 0,
  HTTP_STATUS_NOT_FOUND,
  HTTP_STATUS_INTERNAL_ERROR,
  COUNT_HTTP_STATUS
}HttpStatus;

typedef struct HttpServer HttpServer;

typedef struct{
  Http client;
  bool used;
  String_Buffer buffer;
  const HttpServer *server;

  void *arg;
  HttpRequest request;

  Thread id;
  
}HttpServerThread;

struct HttpServer {
#ifdef _WIN32
  SOCKET socket;
#endif //_WIN32

#ifdef linux  
  int socket;
#endif //linux
  bool running;
  HttpServerThread *threads;
  void (*handle_request)(const HttpRequest *request, Http *client, void *arg);
  void (*handle_websocket)(const char *message, size_t message_len, Http *client, void *arg);
  size_t threads_count;
  bool ssl;
};

typedef enum {
  HTTP_ACCEPT_OK = 0,
  HTTP_ACCEPT_AGAIN,
  HTTP_ACCEPT_ERROR
}HttpAccept;

typedef struct {
  char xormask[4];
  void (*handle_websocket)(const char *message, size_t message_len, Http *client, void *arg);
  unsigned long payload_len;
  Http *http;
  void *arg;
}HttpWebSocketContext;

//----------BEGIN HTTP----------
HTTP_DEF bool http_init(Http *http, const char *url);
HTTP_DEF bool http_init2(Http *http, const char *hostname, size_t hostname_len, bool ssl);
HTTP_DEF bool http_init3(Http *http, const char *hostname, size_t hostname_len, bool ssl, int port);

HTTP_DEF int http_find_hostname(const char *url, size_t url_len, size_t *hostname_len, bool *ssl);
HTTP_DEF const char *http_get_route(const char *url);
HTTP_DEF bool http_sleep_ms(int ms);
HTTP_DEF bool http_encodeURI(const char *input, size_t input_size,
			     char* output, size_t output_cap,
			     size_t *output_size);
HTTP_DEF bool http_decodeURI(const char *input, size_t input_size,
			     char* output, size_t output_cap,
			     size_t *output_size);

HTTP_DEF bool http_read_body(Http *http, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata, HttpHeader *header);

// -- HTTP1 API
HTTP_DEF bool http_request(Http *http, const char *route, const char *method, const char* body, const char *content_type, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata, HttpHeader *header, const char *headers_extra);
HTTP_DEF bool http_get(const char *url, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata, HttpHeader *header, const char *headers_extra);
HTTP_DEF bool http_post(const char *url, const char *body, const char *content_type, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata, HttpHeader *header, const char *headers_extra);
HTTP_DEF bool http_head(const char *url, HttpHeader *header, const char *headers_extra);
HTTP_DEF void http_free(Http *http);

HTTP_DEF bool http_header_has(const HttpHeader *header, string key, string *value);
HTTP_DEF int http_header_response_code(const HttpHeader *header);
//----------END HTTP----------


//----------BEGIN HTTP_SERVER----------
HTTP_DEF bool http_server_init(HttpServer *server, size_t port, const char *cert_file, const char *key_file);
HTTP_DEF bool http_server_listen_and_serve(HttpServer *server, void (*handle_request)(const HttpRequest *request, Http *client, void *arg), size_t number_of_threads, void *args, size_t arg_size_bytes);
HTTP_DEF void http_server_stop(HttpServer *server);
HTTP_DEF void http_server_free(HttpServer *server);

HTTP_DEF void *http_server_listen_function(void *arg);
HTTP_DEF bool http_server_create_serve_thread(HttpServer *server, Http client);
HTTP_DEF void *http_server_serve_function(void *arg);

HTTP_DEF bool http_server_fill_http_request(string request_string, HttpRequest *request, string *websocket_key);

// -- UTIL
HTTP_DEF bool http_send_not_found(Http *http);
HTTP_DEF bool http_send_websocket_accept(Http *http, string websocket_key);

HTTP_DEF const char* http_server_content_type_from_name(string file_name);
HTTP_DEF void http_send_files(Http *client, const char *dir, const char *home, string file_path);

HTTP_DEF bool http_open_file(Io_File *file, const char* file_path);
HTTP_DEF bool http_open_file2(Io_File *file, string file_path);
HTTP_DEF bool http_open_file3(Io_File *file, const char *dir, string file_path);
HTTP_DEF bool http_send_file(Io_File *file, Http *http, const char *add_headers);
HTTP_DEF void http_close_file(Io_File *file);

HTTP_DEF void http_server_simple_file_handler(const HttpRequest *request, Http *client, void *arg);
//----------END HTTP_SERVER----------


//----------BEGIN SOCKET----------
HTTP_DEF SOCKET http_open();
HTTP_DEF bool http_bind(SOCKET socket, int port);
HTTP_DEF void http_shutdown(SOCKET socket);
HTTP_DEF void http_close(SOCKET socket);

HTTP_DEF ssize_t http_send(Http *http, const void *buf, int len);
HTTP_DEF ssize_t http_read(Http *http, void *buf, int count);

HTTP_DEF bool http_valid(SOCKET socket);
HTTP_DEF void http_make_blocking(SOCKET socket);
HTTP_DEF void http_make_nonblocking(SOCKET socket);

HTTP_DEF HttpAccept http_accept(SOCKET server, SOCKET *client);
HTTP_DEF HttpAccept http_select(SOCKET client, fd_set *read_fds, struct timeval *timeout);
HTTP_DEF bool http_connect(SOCKET socket, bool ssl, const char *hostname, size_t hostname_len);
HTTP_DEF bool http_connect2(SOCKET socket, int port, const char *hostname, size_t hostname_len);
HTTP_DEF bool http_respond(Http *http, HttpStatus status, const char *content_type, const void *body, size_t body_size_bytes);

HTTP_DEF bool http_skip_headers(Http *http, char *buf, int count, ssize_t *nbytes_total, int64_t *offset);

HTTP_DEF bool http_send_len(const char *buffer, size_t buffer_len, Http *http);
HTTP_DEF bool http_send_len2(const char *buffer, size_t buffer_len, void *http);

HTTP_DEF bool http_read_len(Http *http, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);

HTTP_DEF void http_init_external_libs(const char *cert_file, const char *key_file);
HTTP_DEF void http_free_external_libs();
//----------END SOCKET----------

//----------BEGIN WEBSOCKET----------
HTTP_DEF bool http_websocket_send(const char *buffer, Http *client);
HTTP_DEF bool http_websocket_send_len(const char *buffer, size_t buffer_len, Http *client);
HTTP_DEF bool http_websocket_read(HttpWebSocketContext *context, String_Buffer *buffer);
//----------END WEBSOCKET----------

HTTP_DEF size_t _fwrite(const void *data, size_t size, size_t memb, void *userdata);

#ifdef HTTP_IMPLEMENTATION

#ifdef _MSC_VER
#  pragma comment(lib,"ws2_32.lib")
#  ifndef HTTP_NO_SSL
#    pragma comment(lib, "crypt32.lib")
#    pragma comment(lib, "advapi32.lib")
#    pragma comment(lib, "user32.lib")
#    pragma comment(lib, "libsslMD.lib")
#    pragma comment(lib, "libcryptoMD.lib")
#  endif //HTTP_NO_SSL
#endif //_WIN32


#ifndef HTTP_NO_SSL
static SSL_CTX *http_global_ssl_client_ctx = NULL;
static SSL_CTX *http_global_ssl_server_ctx = NULL;
#endif //HTTP_NO_SSL
static bool http_global_wsa_startup = false;

//----------BEGIN HTTP----------

HTTP_DEF bool http_init(Http *http, const char *url) {
  if(!http) {
    return false;
  }

  if(!url) {
    return false;
  }

  size_t url_len = strlen(url);
  bool ssl;
  size_t hostname_len;

  int hostname = http_find_hostname(url, url_len, &hostname_len, &ssl);
  if(hostname < 0) {
    warn("can not find hostname");
    return false;
  }
  
  return http_init2(http, url + hostname, hostname_len, ssl);
}

HTTP_DEF bool http_init2(Http *http, const char *hostname, size_t hostname_len, bool ssl) {
  return http_init3(http, hostname, hostname_len, ssl, ssl ? HTTPS_PORT : HTTP_PORT);
}

HTTP_DEF bool http_init3(Http *http, const char *hostname, size_t hostname_len, bool ssl, int port) {
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
    SSL_set_fd(http->conn, (int) http->socket);
  } else {
    http->conn = NULL;
  }
#else
  if(ssl) {
    warn("Compile with openssl and remove HTTP_NO_SSL, to request the url");
    return false;
  }
#endif //HTTP_NO_SSL

  if(!http_connect2(http->socket, port, hostname, hostname_len)) {
    warn("connect failed");
    return false;
  }

#ifndef HTTP_NO_SSL
  int ret;
  if(ssl && (ret = SSL_connect(http->conn)) != 1) {
    if(SSL_get_error(http->conn, ret) == SSL_ERROR_SSL) {
      warn("This is probapliy happening because of HTTP >= 2!");
    }
    return false;
  }
#endif //HTTP_NO_SSL

  http->host = hostname;
  http->host_len = hostname_len;

  return true;    
}

HTTP_DEF int http_find_hostname(const char *url, size_t url_len,
				size_t *hostname_len, bool *ssl) {
  char *http = "http://";
  size_t http_len = 7;
  char* https = "https://";
  size_t https_len = 8;

  int start = cstr_index_of(url, url_len, http, http_len);
  if(start<0) {
    start = cstr_index_of(url, url_len, https, https_len);
    if(start<0) return -1;
    start += (int) https_len;
    if(ssl) *ssl = true;
  }
  else {
    start += (int) http_len;
    if(ssl) *ssl = false;
  }

  int end = cstr_index_of(url + start, url_len - start + 1, "/", 1);
  if(end<0) end = (int) url_len;
  else end += start;

  if(hostname_len) *hostname_len = end - start;
  return start;
}

HTTP_DEF const char *http_get_route(const char *url) {
  size_t url_len = strlen(url);
  bool ssl;
  size_t hostname_len;

  int hostname = http_find_hostname(url, url_len, &hostname_len, &ssl);
  if(hostname < 0) {
    return NULL;
  }

  size_t directory_len = url_len - hostname - hostname_len;
  const char *route = "/";
  if(directory_len>0) {
    route = url + hostname + hostname_len;
  }

  return route;
}

HTTP_DEF bool http_sleep_ms(int ms) {

#ifdef _WIN32
  Sleep(ms);
  return true;
#endif //_WIN32

#ifdef linux
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
#endif //linux
}

HTTP_DEF bool http_encodeURI(const char *input, size_t input_size,
			     char* output, size_t output_cap,
			     size_t *output_size) {

  if(!input || !output || !output_size) {
    return false;
  }

  if(input_size >= output_cap) {
    return false;
  }

  const char *hex = "0123456789abcdef";    
  size_t pos = 0;
  for (size_t i = 0; i < input_size; i++) {
    if (('a' <= input[i] && input[i] <= 'z')
	|| ('A' <= input[i] && input[i] <= 'Z')
	|| ('0' <= input[i] && input[i] <= '9')) {
      output[pos++] = input[i];
    } else if(pos + 3 > output_cap) {
      return false;
    } else {
      output[pos++] = '%';
      output[pos++] = hex[(input[i] & 0xf0) >> 4];
      output[pos++] = hex[input[i] & 0xf];
    }
  }

  *output_size = pos;

  return true;  
}

HTTP_DEF bool http_decodeURI(const char *input, size_t input_size,
			     char* output, size_t output_cap,
			     size_t *output_size) {
  if(!input || !output || !output_size) {
    return false;
  }

  size_t pos = 0;
  for(size_t i=0;i<input_size;i++) {
    if(pos >= output_cap) {
      return false;
    }
    
    char c = input[i];
    if(c == '%') {
      if(i + 2 >= input_size) {	
	return false;
      }
      char hi = input[i+1];
      if(hi > 57) hi -= 39;
      char lo = input[i+2];
      if(lo > 57) lo -= 39;
      output[pos++] = (hi << 4) | (lo & 0xf);
      i+=2;
    } else if(c == '\\' && i+5 < input_size) {
      bool replace_with_and = true;
      if(input[i+1] != 'u') replace_with_and = false;
      if(input[i+2] != '0') replace_with_and = false;
      if(input[i+3] != '0') replace_with_and = false;
      if(input[i+4] != '2') replace_with_and = false;
      if(input[i+5] != '6') replace_with_and = false;
      if(replace_with_and) {
	output[pos++] = '&';
	i += 5;
      }
    } else {
      output[pos++] = c;
    }
  }

  *output_size = pos;

  return true;
}

HTTP_DEF bool http_request(Http *http, const char *route, const char *method,
			   const char* body, const char *content_type,
			   size_t (*write_callback)(const void*, size_t, size_t, void *),
			   void *userdata, HttpHeader *header, const char *headers_extra)
{
  bool hasBody = body != NULL && content_type != NULL;

  //SEND
  char request_buffer[HTTP_BUFFER_CAP];
  if(!hasBody) {
    if(!sendf(http_send_len2, http, request_buffer, HTTP_BUFFER_CAP,
	      "%s %s HTTP/1.1\r\n"
	      "Host: %.*s\r\n"
	      "%s"
	      "\r\n", method, route, http->host_len, http->host, headers_extra == NULL ? "" : headers_extra)) {
      warn("send failed");
      return false;
    }
  } else {
    if(!sendf(http_send_len2, http, request_buffer, HTTP_BUFFER_CAP,
	      "%s %s HTTP/1.1\r\n"
	      "Host: %.*s\r\n"
	      "Content-Type: %s\r\n"
	      "Content-Length: %d\r\n"
	      "%s"
	      "\r\n"
	      "%s", method, route, http->host_len, http->host, content_type, strlen(body), headers_extra == NULL ? "" : headers_extra, body)) {
      warn("send failed");
      return false;
    }    
  }

  //READ
  if(!http_read_body(http, write_callback, userdata, header)) {
    warn("read failed"); fflush(stdout);
    return false;
  }

  return true;
}

HTTP_DEF bool http_get(const char *url, size_t (*write_callback)(const void *, size_t,size_t, void *), void *userdata, HttpHeader *header, const char *headers_extra) {
  
  size_t url_len = strlen(url);
  bool ssl;
  size_t hostname_len;

  int hostname = http_find_hostname(url, url_len, &hostname_len, &ssl);
  if(hostname < 0) {
    warn("can not find hostname");
    return false;
  }

  size_t directory_len = url_len - hostname - hostname_len;
  const char *route = "/";
  if(directory_len>0) {
    route = url + hostname + hostname_len;
  }

  Http http;
  if(!http_init2(&http, url + hostname, hostname_len, ssl)) {
    warn("http_init failed");
    return false;
  } 

  if(!http_request(&http, route, "GET", NULL, NULL, write_callback, userdata, header, headers_extra)) {
    warn("http_request failed");
    return false;
  }

  http_free(&http);

  return true;
}

HTTP_DEF bool http_post(const char *url, const char *body, const char *content_type, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata, HttpHeader *header, const char *headers_extra) {
  size_t url_len = strlen(url);
  bool ssl;
  size_t hostname_len;

  int hostname = http_find_hostname(url, url_len, &hostname_len, &ssl);
  if(hostname < 0) {
    return false;
  }

  size_t directory_len = url_len - hostname - hostname_len;
  const char *route = "/";
  if(directory_len>0) {
    route = url + hostname + hostname_len;
  }

  Http http;
  if(!http_init2(&http, url + hostname, hostname_len, ssl)) {
    return false;
  }

  ///////////////////////////

  if(!http_request(&http, route, "POST", body, content_type, write_callback, userdata, header, headers_extra)) {
    return false;
  }

  http_free(&http);

  return true;
}

HTTP_DEF bool http_header_has(const HttpHeader *_header, string key, string *value) {
  if(!_header) {
    return false;
  }

  string header = string_from(_header->data, _header->size);
  int pos = string_index_of2(header, key);
  if(pos < 0) {
    return false;
  }

  if(value) {
    string temp = string_from(header.data + pos + key.len + 1, header.len - pos - key.len - 1);
    *value = string_trim(string_chop_by_delim(&temp, '\n'));
  }
  
  return true;
}

HTTP_DEF int http_header_response_code(const HttpHeader *_header) {
  string header = string_from(_header->data, _header->size);
  string line = string_trim(string_chop_by_delim(&header, '\n'));
  string_chop_left(&line, 9);
  int n;
  string_chop_int(&line, &n);
  return n;
}

HTTP_DEF bool http_head(const char *url, HttpHeader *header, const char *headers_extra) {
  size_t url_len = strlen(url);
  bool ssl;
  size_t hostname_len;

  int hostname = http_find_hostname(url, url_len, &hostname_len, &ssl);
  if(hostname < 0) {
    return false;
  }

  size_t directory_len = url_len - hostname - hostname_len;
  const char *route = "/";
  if(directory_len>0) {
    route = url + hostname + hostname_len;
  }

  Http http;
  if(!http_init2(&http, url + hostname, hostname_len, ssl)) {
    return false;
  }

  char request_buffer[HTTP_BUFFER_CAP];
  if(!sendf(http_send_len2, &http, request_buffer, HTTP_BUFFER_CAP,
	    "HEAD %s HTTP/1.1\r\n"
	    "Host: %.*s\r\n"
	    "%s"
	    "Connection: Close\r\n"
	    "\r\n",
	    route, http.host_len,
	    http.host, headers_extra == NULL ? "" : headers_extra)) {
    return false;
  }

  if(!http_read_body(&http, NULL, NULL, header)) {
    return false;
  }

  http_free(&http);

  return true;
}

HTTP_DEF void http_free(Http *http) {
  if(http_valid(http->socket)) {
    http_shutdown(http->socket);
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
HTTP_DEF bool http_server_init(HttpServer *server, size_t port, const char *cert_file, const char *key_file) {

  if(server == NULL) {
    return false;
  }

  server->threads = NULL;
  server->threads_count = 0;
  server->ssl = cert_file != NULL && key_file != NULL;
  server->handle_websocket = NULL;
  server->running = false;

  http_init_external_libs(cert_file, key_file);

  server->socket = http_open();
  if(!http_valid(server->socket)) {
    return false;
  }

  //Potential Fix for TIME_WAIT
#ifdef linux
  const int enable = 1;
  if(setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))<0) {
    return false;
  }

  if(setsockopt(server->socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int))<0) {
    return false;
  }
#endif //linux

  if(!http_bind(server->socket, (int) port)) {
    return false;
  }

  http_make_nonblocking(server->socket);


  return true;
}

HTTP_DEF bool http_server_listen_and_serve(HttpServer *server, void (*handle_request)(const HttpRequest *, Http *, void *), size_t number_of_threads, void *_args, size_t arg_size_bytes) {
  if(server == NULL) {
    return false;
  }

  if(number_of_threads == 0) {
    return false;
  }

  if(handle_request == NULL) {
    return false;
  }

  char *args = (char *) _args;

  bool args_used = args != NULL;
  bool arg_singleton = args_used && arg_size_bytes == 0;

  //ALLOCATE THREADS
  server->threads_count = number_of_threads;
  server->threads = (HttpServerThread *) malloc(server->threads_count*sizeof(HttpServerThread));
  if(!server->threads) {
    return false;
  }
  server->handle_request = handle_request;

  //INITIALIZE THREADS
  for(size_t i=0;i<number_of_threads;i++) {
    HttpServerThread *thread = &server->threads[i];
    thread->used = false;
    thread->buffer = (String_Buffer) {0};
    thread->server = server;
    if(!args_used) {
      thread->arg = NULL;
    } else if(arg_singleton) {
      thread->arg = args;
    } else {
      thread->arg = args + arg_size_bytes*i;
    }
  }

  //START LISTEN THREAD
  if(listen(server->socket, 3) < 0) {
    free(server->threads);
    return false;
  }

  server->threads[0].used = true;

  if(!thread_create(&server->threads[0].id, http_server_listen_function, server)) {
    free(server->threads);
    return false;
  }
  
  server->running = true;

  return true;
}

HTTP_DEF void http_server_stop(HttpServer *server) {
  if(server == NULL) {
    return;
  }

  server->running = false;

  //JOIN ACCEPT THREAD
  for(size_t i=1;i<server->threads_count;i++) {
    HttpServerThread *thread = &(server->threads[i]);
    if(thread->used == false) {
      continue;
    }
    http_free(&thread->client);

    thread_join(thread->id);

    thread->used = false;
    string_buffer_free(&(thread->buffer));
  }

  //STOP LISTEN THREAD
  server->threads[0].used = false;
  thread_join(server->threads[0].id);  

  //DEALLOCATE THREADS
  if(server->threads_count == 0) {
    return;
  }
  if(server->threads) {
    free(server->threads);
  }
}

HTTP_DEF void http_server_free(HttpServer *server) {
  if(server == NULL) {
    return;
  }

  if(!http_valid(server->socket)) {
    return;
  }
  
  if(server->threads!=NULL && server->threads[0].used == true) {
    http_server_stop(server);
  }

  http_shutdown(server->socket);
  //http_make_blocking(server->socket);
  http_close(server->socket);
}

HTTP_DEF void *http_server_listen_function(void *arg) {
  HttpServer *server = (HttpServer *) arg;
  bool *running = &(server->threads[0].used);

  while(*running) {
    SOCKET client;
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

	SSL_set_fd(http.conn, (int) client);
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

HTTP_DEF bool http_server_create_serve_thread(HttpServer *server, Http client) {  
  for(size_t i=1;i<server->threads_count;i++) {
    HttpServerThread *thread = &(server->threads[i]);
    if(thread->used==true) continue;
    thread->client = client;

    if(!thread_create(&thread->id, http_server_serve_function, thread)) {
      return false;
    }

    thread->used = true;
    return true;
  }

  return false;
}

HTTP_DEF void *http_server_serve_function(void *_arg) {
  HttpServerThread *thread = (HttpServerThread *) _arg;
  Http client = thread->client;
  bool *used = &(thread->used);
  String_Buffer *buffer = &(thread->buffer);
  void (*handle_request)(const HttpRequest *, Http *, void *) = thread->server->handle_request;
  void (*handle_websocket)(const char *, size_t, Http *, void *) = thread->server->handle_websocket;
  HttpRequest *request = &(thread->request);
  *request = (HttpRequest) {0};
  void* arg = thread->arg;

  http_make_nonblocking(client.socket);

  fd_set read_fds = {0};
  FD_SET(client.socket, &read_fds);

  struct timeval timeout;
  timeout.tv_sec = 2;
  timeout.tv_usec = 0;

  HttpWebSocketContext websocket = {0};
  websocket.payload_len = 0;
  websocket.handle_websocket = handle_websocket;
  websocket.http = &client;
  websocket.arg = arg;

  while(true) {
    buffer->len = 0;
    HttpAccept select = http_select(client.socket, &read_fds, &timeout);
    if(select != HTTP_ACCEPT_OK) {
      break;
    }

    if(!http_read_len(&client, string_buffer_callback, buffer)) {
      break;
    }

    //PROPER HTTP REQUEST
    string websocket_key;
    if(http_server_fill_http_request(string_from(buffer->data, buffer->len), request, &websocket_key)) {
      if(websocket_key.len > 0 && handle_websocket != NULL) {
	http_send_websocket_accept(&client, websocket_key);
      } else {
	handle_request(request, &client, arg);
      }
    } else if(handle_websocket != NULL) {
      if(!http_websocket_read(&websocket, buffer)) {
	break;
      }
    }    
  } 

  http_free(&client);
  *used = false;

  return NULL;
}

//TODO: Validate HttpRequest properly, to distinguish between Websocket and httpRequest
HTTP_DEF bool http_server_fill_http_request(string request_string, HttpRequest *request, string *websocket_key) {
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
    if(line.len == request_string_origin.len) {
      return false;
    }

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

  if(bodyStart != 0) {
    request->body = string_from(request_string_origin.data + bodyStart,
				request_string_origin.len - bodyStart);
    request->headers = string_trim(string_from(request_string_origin.data + headersStart,
					       bodyStart - headersStart));      
  } else {    
    request->headers = string_trim(string_from(request_string_origin.data + headersStart,
					       request_string_origin.len));
  }

  if(websocket_key) {
    *websocket_key = (string) {0};
    int udgrade_key = string_index_of(request->headers, "Upgrade:");
    if(udgrade_key == -1) {
      return true;
    }

    int upgrade_value = string_index_of_offset(request->headers, ": websocket", udgrade_key);
    if(upgrade_value == -1) {
      return true;
    }

    if(upgrade_value - udgrade_key != 7) {
      return true;
    }

    int sec_key = string_index_of(request->headers, "Sec-WebSocket-Key: ");
    if(sec_key == -1) {
      return true;
    }

    size_t i = sec_key + 21;
    if(i >= request->headers.len) {
      return true;
    }

    while(i<request->headers.len && request->headers.data[i]!='\r') i++;
    *websocket_key = string_from(request->headers.data + sec_key + 19, i - sec_key - 19);
  }

  return true;
}

HTTP_DEF bool http_send_not_found(Http *http) {
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

HTTP_DEF bool http_send_websocket_accept(Http *http, string ws_key) {
  const char *guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  unsigned char result[20];

  _SHA1_CTX ctx;
  _SHA1Init(&ctx);
  _SHA1Update(&ctx, (unsigned char *) ws_key.data, (int) ws_key.len);
  _SHA1Update(&ctx, (unsigned char *) guid, (int) strlen(guid));
  _SHA1Final((unsigned char *) result, &ctx);

  size_t size;
  char buffer[128];
  if(!base64_encode((const char *) result, 20, buffer, 80, &size)) {
    panic("base64_encode");
  }

  char request_buffer[HTTP_BUFFER_CAP];
  return sendf(http_send_len2, http, request_buffer, HTTP_BUFFER_CAP,
	       "HTTP/1.1 101 Switching Protocols\r\n"
	       "Upgrade: websocket\r\n"
	       "Connection: Upgrade\r\n"
	       "Sec-WebSocket-Accept: %.*s\r\n"
	       "\r\n", size, buffer);
}

static const char* HTTP_CONTENT_TYPE_TEXT_PLAIN = "text/plain; charset=utf-8";
static const char* HTTP_CONTENT_TYPE_HTML = "text/html; charset=utf-8";
static const char* HTTP_CONTENT_TYPE_JS = "application/javascript; charset=utf-8";
static const char* HTTP_CONTENT_TYPE_JSON = "application/json; charset=utf-8";
static const char* HTTP_CONTENT_TYPE_CSS = "text/css; charset=utf-8";
static const char* HTTP_CONTENT_TYPE_WASM = "application/wasm";

HTTP_DEF const char* http_server_content_type_from_name(string file_name) {
  const char *content_type = HTTP_CONTENT_TYPE_TEXT_PLAIN;

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
  } else if(string_eq(ext, STRING(".css"))) {
    content_type = HTTP_CONTENT_TYPE_CSS;
  } else if(string_eq(ext, STRING(".wasm"))) {
    content_type = HTTP_CONTENT_TYPE_WASM;
  }else if(string_eq(ext, STRING(".json"))) {
    content_type = HTTP_CONTENT_TYPE_JSON;
  } else {
    fprintf(stderr, "[WARNING] Unknown extension: "String_Fmt"\n", String_Arg(ext));
  }

  return content_type;
}

HTTP_DEF void http_send_files(Http *client, const char *dir, const char *home, string file_path) {
  
  if(string_eq(file_path, STRING("/"))) {
    file_path = string_from_cstr(home);
  }

  Io_File file;
  if(!http_open_file3(&file, dir, file_path)) {
    http_send_not_found(client);
    return;
  }
  http_send_file(&file, client, http_server_content_type_from_name(file_path));
  http_close_file(&file);
}

HTTP_DEF bool http_open_file(Io_File *file, const char *file_path) {
  if(!io_file_open(file, file_path)) {
    return false;
  }

  return true;
}

HTTP_DEF bool http_open_file2(Io_File *file, string file_path) {
#ifdef _WIN32
  char buffer[MAX_PATH+1];
  assert(file_path.len < MAX_PATH);
#endif //_WIN32
#ifdef linux
  char buffer[PATH_MAX+1];
  assert(file_path.len < PATH_MAX);
#endif //linux  
  memcpy(buffer, file_path.data, file_path.len);
  buffer[file_path.len] = 0;
  return http_open_file(file, buffer);
}

HTTP_DEF bool http_open_file3(Io_File *file, const char *dir, string file_path) {
  size_t dir_len = strlen(dir);
  if(dir_len == 0) {
    return false;
  }

#ifdef _WIN32
  char buffer[MAX_PATH+1];
  assert(file_path.len + dir_len < MAX_PATH);
#endif //_WIN32
#ifdef linux
  char buffer[PATH_MAX+1];
  assert(file_path.len + dir_len < PATH_MAX);
#endif //linux

  memcpy(buffer, dir, dir_len);
  memcpy(buffer + dir_len, file_path.data, file_path.len);
  buffer[dir_len + file_path.len] = 0;

  return http_open_file(file, buffer);
}

HTTP_DEF bool http_send_file(Io_File *file, Http *http, const char *content_type) {
  if(!http_valid(http->socket)) {
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
    //size_t nbytes = fread(buffer + buffer_off + 6, 1, -6 + buffer_cap - 2, *f);
    size_t nbytes = io_file_fread(buffer + buffer_off + 6, 1, -6 + buffer_cap - 2, file);
    if(nbytes != buffer_cap - 2 - 6) {
      if(io_file_ferror(file)) {
	//TODO: error handling
	//fprintf(stderr, "[WARNING]: Can not read file because: %s\n", strerror(errno));
	return false;
      }
      if(io_file_feof(file)) {
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
	char m = (char) (n % 16);
	buffer[buffer_off + 4 - i++ - 1] = m<10 ? m + '0' : m + 'W';
	n /= 16;
      }
      while(i < 4) buffer[buffer_off + 4 - i++ - 1] = '0';

      if(bbreak && (6 + nbytes + 2 + end_carriage_len < buffer_cap)) {
	memcpy(buffer + buffer_off + 6 + nbytes + 2, end_carriage, end_carriage_len);
	nbytes += end_carriage_len;
      }
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

HTTP_DEF void http_close_file(Io_File *file) {
  io_file_close(file);
}

HTTP_DEF ssize_t http_send(Http *http, const void *buf, int len) {
#ifndef HTTP_NO_SSL
  if(http->conn != NULL) {
    return SSL_write(http->conn, buf, len);
  } else {
    return send(http->socket, buf, len, 0);
  }
#else
  return send(http->socket, buf, len, 0);
#endif //HTTP_NO_SSL 
}

HTTP_DEF ssize_t http_read(Http *http, void *buf, int count) {
#ifndef HTTP_NO_SSL  
  if(http->conn != NULL) {
    return SSL_read(http->conn, buf, count);
  }
  else {
    return recv(http->socket, buf, count, 0);
  }
#else
  return recv(http->socket, buf, count, 0);
#endif //HTTP_NO_SSL
}

HTTP_DEF void http_server_simple_file_handler(const HttpRequest *request, Http *client, void *arg) {
  (void) arg;

  if(string_eq(request->method, STRING("GET"))) {
    http_send_files(client, ".", "/index.html", request->route);
    return;
  }

  http_send_not_found(client);
}
//----------END HTTP_SERVER----------


//----------BEGIN SOCKET----------
HTTP_DEF SOCKET http_open() {
#ifdef _WIN32
  return WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
#endif //_WIN32
#ifdef linux
  struct protoent *protoent = getprotobyname("tcp");
  if(!protoent) {
    return -1;
  }

  return socket(AF_INET, SOCK_STREAM, /*protoent->p_proto*/ 0);
#endif //linux
}

HTTP_DEF bool http_bind(SOCKET socket, int port) {
  if(!http_valid(socket)) {
    return false;
  }

#ifdef _WIN32
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons((u_short) port);

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

HTTP_DEF void http_shutdown(SOCKET socket) {
#ifdef _WIN32
  shutdown(socket, SD_BOTH);
#endif //_WIN32
#ifdef linux
  shutdown(socket, SHUT_RDWR);
#endif //linux
}

HTTP_DEF void http_close(SOCKET socket) {
#ifdef _WIN32
  closesocket(socket);
#endif //_WIN32
#ifdef linux
  close(socket);
#endif //linux
}


HTTP_DEF bool http_valid(SOCKET socket) {
#ifdef _WIN32
  return (socket) != INVALID_SOCKET;
#endif //_WIN32
#ifdef linux
  return socket !=-1;
#endif //linux
}

HTTP_DEF void http_make_blocking(SOCKET socket) {
#ifdef _WIN32
  (void) socket;
#endif //_WIN32
#ifdef linux
  int flags = fcntl(socket, F_GETFL, 0);
  fcntl(socket, F_SETFL, flags & ~O_NONBLOCK);
#endif //linux
}

HTTP_DEF void http_make_nonblocking(SOCKET socket) {
#ifdef _WIN32
  unsigned long mode = 1;
  ioctlsocket(socket, FIONBIO, &mode);
#endif //_WIN32
#ifdef linux
  int flags = fcntl(socket, F_GETFL, 0);
  fcntl(socket, F_SETFL, flags | O_NONBLOCK);
#endif //linux
}

HTTP_DEF HttpAccept http_accept(SOCKET socket, SOCKET *out_client) {
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

#if 0
  //LOG: ip address
  struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&addr;
  struct in_addr ipAddr = pV4Addr->sin_addr;
  struct sockaddr_in6* pV6Addr = (struct sockaddr_in6*)&addr;
  struct in6_addr ipAddr6       = pV6Addr->sin6_addr;

  char str_ipv4[INET_ADDRSTRLEN];
  inet_ntop( AF_INET, &ipAddr, str_ipv4, INET_ADDRSTRLEN );
  char str_ipv6[INET6_ADDRSTRLEN];
  inet_ntop( AF_INET6, &ipAddr6, str_ipv6, INET6_ADDRSTRLEN );
  printf("IPv4: %s, IPv6: %s\n", str_ipv4, str_ipv6);
#endif //HTTP_DEBUG

  if(out_client) *out_client = client;
  return HTTP_ACCEPT_OK;
}

HTTP_DEF HttpAccept http_select(SOCKET client, fd_set *read_fds, struct timeval *timeout) {
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

HTTP_DEF bool http_connect(SOCKET socket, bool ssl, const char *hostname, size_t hostname_len) {
  return http_connect2(socket, ssl ? HTTPS_PORT : HTTP_PORT, hostname, hostname_len);
}

HTTP_DEF bool http_connect2(SOCKET socket, int port, const char *hostname, size_t hostname_len) {
  if(!http_valid(socket)) {
    return false;
  }
  if(hostname == NULL) {
    return false;
  }

#ifdef linux
  struct sockaddr_in addr = {0};
  char name[hostname_len + 1];
  memcpy(name, hostname, hostname_len);
  name[hostname_len] = 0;

  struct hostent *hostent = gethostbyname(name);
  if(!hostent) {
    return false;
  }

  in_addr_t in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
  if(in_addr == (in_addr_t) -1) {
    return false;
  }
  addr.sin_addr.s_addr = in_addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons((u_short) port);
  if(connect(socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    return false;
  }
  
#elif _WIN32
  char *name = (char *) alloca(hostname_len + 1);
  memcpy(name, hostname, hostname_len);
  name[hostname_len] = 0;

  struct addrinfo hints;
  struct addrinfo* result = NULL;

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  char port_cstr[6];
  snprintf(port_cstr, sizeof(port_cstr), "%d", port);

  if(getaddrinfo(name, port_cstr, &hints, &result) != 0) {
    return false;
  }

  if(connect(socket, result->ai_addr, (int) result->ai_addrlen) != 0) {
    freeaddrinfo(result);
    return false;
  }

  freeaddrinfo(result);
#endif //linux - _WIN32

  return true; 
}

HTTP_DEF bool http_respond(Http *http, HttpStatus status, const char *content_type, const void *body, size_t body_size_bytes) {
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

  static_assert(COUNT_HTTP_STATUS == 3, "The amount of HttpStatus has changed.");
  switch(status) {
  case HTTP_STATUS_OK:
    prefix = "HTTP/1.1 200 OK";
    break;
  case HTTP_STATUS_NOT_FOUND:
    prefix = "HTTP/1.1 404 Not Found";
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

HTTP_DEF bool http_skip_headers(Http *http, char *buf, int count, ssize_t *nbytes_total, int64_t *offset) {
  do {
    *nbytes_total = http_read(http, buf, count);
    if(*nbytes_total < 0 ) {
      return false;
    }

    string s = string_from(buf, (size_t) *nbytes_total);
    while(s.len) {
      string line = string_chop_by_delim(&s, '\n');
      if(line.len && line.data[0]=='\r') {
	*offset = (int64_t) (line.data+2 - buf);
	return true;
      }
    }

  } while(*nbytes_total > 0);

  return false;
}

HTTP_DEF bool http_send_len(const char *buffer, size_t _buffer_size, Http *http) {
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
    nbytes_last = http_send(http, buffer + nbytes_total, (int) (buffer_size - nbytes_total) );

    if(nbytes_last == -1) {
      return false;
    }
    nbytes_total += nbytes_last;
  }

  return true;
}

HTTP_DEF bool http_send_len2(const char *buffer, size_t buffer_len, void *http) {
  return http_send_len(buffer, buffer_len, (Http *) http);
}

HTTP_DEF bool http_read_len(Http *http, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata) {
  if(!http_valid(http->socket)) {
    return false;
  }  

  char buffer[HTTP_BUFFER_CAP];

  ssize_t nbytes_total = 0;
  ssize_t nbytes_last;
  
  do {
    nbytes_last = http_read(http, buffer, HTTP_BUFFER_CAP);

#ifdef linux
    //TODO: check if it should <= 0 OR < 0
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

    //TODO: test that
  }while(nbytes_last > 0);

  return false;
}

HTTP_DEF bool http_read_body(Http *http, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata),
			     void *userdata, HttpHeader *header) {
  if(!http_valid(http->socket)) {
    return false;
  }

  if(header) {
    header->size = 0;
  }

  char buffer[HTTP_BUFFER_CAP];
  int64_t content_length = -1;
  int64_t need = -1;
  uint64_t read = 0;

  bool body = false;

  ssize_t nbytes_total;
  do{
    nbytes_total = http_read(http, buffer, HTTP_BUFFER_CAP);

    if(nbytes_total == -1) {      
      return false;
    }

    size_t offset = 0;
    if(!body) {
      string s = string_from(buffer, (size_t) nbytes_total);
      while(s.len) {
	string line = string_chop_by_delim(&s, '\n');
	if(line.len && line.data[0]=='\r') {
	  body = true;
	  offset = (size_t) (line.data+2 - buffer);
	  break;
	}
	string key = string_trim(string_chop_by_delim(&line, ':'));
	string value = string_trim(line);
#ifdef HTTP_DEBUG
	printf(String_Fmt": "String_Fmt"\n", String_Arg(key), String_Arg(value)); fflush(stdout);
#endif	
	if(string_eq(key, STRING("Content-Length")) ||
	   string_eq(key, STRING("content-length"))) {
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

      if(header) {
	size_t header_additional_len = offset == 0 ? (size_t) nbytes_total : offset;
	assert(header->size + header_additional_len < HTTP_BUFFER_CAP);
	memcpy(header->data + header->size, buffer, header_additional_len);
	header->size += header_additional_len;

      }
    }

    if(body) {
      if(write_callback == NULL) {
	break;
      }

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
	bool bbreak = false;
	size_t diff = (size_t) nbytes_total - offset;
	if(diff==0) {
	  continue;
	}
	size_t off = 0;
	size_t i = 0;

	char *buf = buffer + offset;
		
	uint64_t n;
	size_t size;
	string word;
	
	for(;i<diff-1;i++) {
	  if(buf[i]!='\r') continue;
	  if(buf[i+1]!='\n') continue;
	  size = i - off;
	  if(size == 0) continue;
	  int64_t bar = (int64_t) size;
	  if(need <= 0) {
	    size = size > 4 ? 4 : size;
	    word = string_from(buf + i - size, size);
	    string_chop_hex(&word, &n);
	    need = (int64_t) n;
	    off = i + 2;
	    if(need == 0) {
	      bbreak = true;
	      break;
	    }
	  } else if(need - bar == 0) {
	    write_callback(buf + off, size, 1, userdata);
	    need = 0;
	    off = i + 2;
	  }		    
	}
	if(need > 0) {		    
	  int64_t bar = (int64_t) i + 1 - off;
	  if(!(need - bar >= 0)) {
	    //printf("need: %ld, i: %ld, off: %lu\n", need, i, off);
	    panic("assert");
	  }
	  write_callback(buf + off, bar, 1, userdata);
	  need -= bar;
	}
	if(bbreak) {
	  break;
	}
      } else {
	warn("Server does not provide a content-length and does not support Chunked Encoding");
	break;
      }

    }
  }while(nbytes_total > 0);

  return true;
}

HTTP_DEF void http_init_external_libs(const char *cert_file, const char *key_file) {

#ifndef HTTP_NO_SSL
  if(!http_global_ssl_client_ctx) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    http_global_ssl_client_ctx = SSL_CTX_new(TLS_client_method());
    if(!http_global_ssl_client_ctx) {
      ERR_print_errors_fp(stderr);
      panic("Can not initialize libopenssl");
    }
    
    http_global_ssl_server_ctx = SSL_CTX_new(TLS_server_method());

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

HTTP_DEF void http_free_external_libs() {
#ifndef HTTP_NO_SSL
    SSL_CTX_free(http_global_ssl_server_ctx);
    SSL_CTX_free(http_global_ssl_client_ctx);
#endif //HTTP_NO_SSL

#ifdef _WIN32
    WSACleanup();
#endif //_WIN32
}
//----------END SOCKET----------

HTTP_DEF size_t _fwrite(const void *data, size_t size, size_t memb, void *userdata) {
  return fwrite(data, size, memb, (FILE *) userdata);
}

//----------BEGIN WEBSOCKET----------
HTTP_DEF bool http_websocket_read(HttpWebSocketContext *context, String_Buffer *buffer) {
  // -- https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers
  if(buffer->len == 0) {
    return true;
  }

  if(!context) {
    return false;
  }

  if(context->payload_len != 0 && context->payload_len <= buffer->len) {
    size_t mask_off = (context->payload_len % 4);
    for(unsigned long i=0;i<buffer->len;i++) {
      buffer->data[i] = buffer->data[i] ^ (context->xormask[(mask_off + i)%4]);
    }
    context->handle_websocket(buffer->data, buffer->len, context->http, context->arg);
    context->payload_len -= (int) buffer->len;
    return true;
  }

  assert(buffer->len > 0);
  unsigned int opcode = buffer->data[0] & 15;	

  if(opcode == 0x08) {
    return false;
  }

  //H := 48 := 0000 1100
  size_t off = 0;
  assert(buffer->len >= 2);
  context->payload_len = buffer->data[1] & 127;

  if(context->payload_len==126 || context->payload_len==127) {
    off = context->payload_len == 126 ? 2 : 8;

    unsigned long _m = 1;
    context->payload_len = 0;
    assert(buffer->len >= off + 2);
    for(int n=(int) off-1;n>=0;n--) {
      for(int i=0;i<8;i++) {
	if((buffer->data[2+n] & (1 << i))) context->payload_len += _m;
	_m = _m << 1;
      }
    }	  
  }
  off += 2;

  assert(buffer->len >= off + 4);	
  for(int i=0;i<4;i++) {
    context->xormask[i] = buffer->data[off+i];
  }
  off += 4;

  unsigned long diff = (unsigned long) buffer->len - (unsigned long) off;

  if(diff > 0) {
    for(unsigned long i=0;i<diff;i++) {
      buffer->data[off + i] = buffer->data[off+i] ^ (context->xormask[i%4]);
    }
    context->handle_websocket(buffer->data + off, diff, context->http, context->arg);
    context->payload_len -= diff;
  }

  return true;
}

HTTP_DEF bool http_websocket_send_len(const char *buffer, size_t buffer_len, Http *client) {
  if(!buffer) {
    return false;
  }

  if(!http_valid(client->socket)) {
    return false;
  }

  char header = -127;
  char xormask[4];
  char res_buffer[HTTP_BUFFER_CAP];
  if(buffer_len < 126) {    
    char size = (char) buffer_len | 0x80;
    return sendf(http_send_len2, client, res_buffer, HTTP_BUFFER_CAP, "%c%c%.*s%_ws",
		 header, size, (size_t) 4, xormask, buffer_len, buffer, xormask);
  } else if(buffer_len <= UINT16_MAX) {    
    int indication = 0xff & ~0x1;
    char size[] = {(char) ((buffer_len & 0xff00) >> 8), (char) (buffer_len & 0x00ff)};    
    return sendf(http_send_len2, client, res_buffer, HTTP_BUFFER_CAP, "%c%d%.*s%.*s%_ws",
		 header, indication, (size_t) 2, size, (size_t) 4, xormask, buffer_len, buffer, xormask);
  } else {
    char indication = -1;
    char size[8];
    for(int i=0;i<8;i++) {
      size[8 - 1 - i] = (buffer_len & (0xff << i*8)) >> i*8;
    }
    return sendf(http_send_len2, client, res_buffer, HTTP_BUFFER_CAP, "%c%c%.*s%.*s%_ws",
		 header, indication, (size_t) 8, size, (size_t) 4, xormask, buffer_len, buffer, xormask);
  }
}

HTTP_DEF bool http_websocket_send(const char *buffer, Http *client) {
  if(!buffer) {
    return false;
  }
  return http_websocket_send_len(buffer, strlen(buffer), client);
}
//----------END WEBSOCKET----------

#endif //HTTP_IMPLEMENTATION

#endif //HTTP_H
