#include <stdio.h>
#include "sha1.h"

void print_bytes(void *_ptr, size_t number_of_bytes) {
  long ptr = * (long *) _ptr;
  printf("\t");
  for(size_t i=0;i<number_of_bytes;i++) {
    for(int j=0;j<8;j++) {
      printf("%d ", (ptr & (1 << (i*8+j))) > 0);
    }
    if(i!=number_of_bytes-1) printf("| ");
  }
  printf("\n");
}

#define BASE64_IMPLEMENTATION
#include "../libs/base64.h"

#define HTTP_IMPLEMENTATION
#define HTTP_NO_SSL
#include "../libs/http.h"

static char cwd[PATH_MAX];
static const char *guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

void handle(const HttpRequest *request, Http *client, void *arg) {
  (void) arg;

  string copy = request->headers;
  string ws_key = {0};

  bool upgradeWebsocket = false;
  while(copy.len) {
    string line = string_trim(string_chop_by_delim(&copy, '\n'));
    if(!line.len) continue;
    string key = string_chop_by_delim(&line, ':');
    string value = string_trim(line);
    if(string_eq(key, STRING("Upgrade")) && string_eq(value, STRING("websocket"))) {
      upgradeWebsocket = true;
    } else if(string_eq(key, STRING("Sec-WebSocket-Key"))) {
      ws_key = value;
    }
  }


  if(upgradeWebsocket && ws_key.len) {
    unsigned char result[21];
    
    SHA1_CTX ctx;
    SHA1Init(&ctx);
    SHA1Update(&ctx, (unsigned char *) ws_key.data, ws_key.len);
    SHA1Update(&ctx, (unsigned char *) guid, strlen(guid));
    SHA1Final((unsigned char *) result, &ctx);

    size_t size;
    char buffer[128];
    if(!base64_encode((const char *) result, 20, buffer, 80, &size)) {
      panic("base64_encode");
    }

    http_sendf(client, "HTTP/1.1 101 Switching Protocols\r\n"
	       "Upgrade: websocket\r\n"
	       "Connection: Upgrade\r\n"
	       "Sec-WebSocket-Accept: %.*s\r\n"
	       "\r\n", size, buffer);
    return;
  }
  
  if(string_eq(request->method, STRING("GET"))) {
    http_send_files(client, cwd, "/index.html", request->route);
    return;
  }

  http_send_not_found(client);
}

void handle_websocket(const char *message, size_t message_size, Http *client, void *arg) {
  (void) client;
  (void) arg;
  printf("Received: '%.*s' (%ld)\n", (int) message_size, message, message_size);

  char buf[65600];
  memset(buf, 'f', 65600);

  http_websocket_send_len(buf, 65600, client);
}

int main(int argc, char **argv) {
  int port = 6969;

  if(argc > 1) {
    string arg = string_from_cstr(argv[1]);
    if(!string_chop_int(&arg, &port)) {
      fprintf(stderr, "ERROR:\tPlease provide a valid port, or use the default one.\n");
      fprintf(stderr, "USAGE:\thttpserver [<port>]\n");
      exit(1);
    }
  }
  
  HttpServer server;
  if(!http_server_init(&server, port, NULL, NULL)) {
    panic("http_server_init");
  }

  if(!getcwd(cwd, sizeof(cwd))) {
    panic("getcwd");
  }

  server.handle_websocket = handle_websocket;
  if(!http_server_listen_and_serve(&server, handle, 24, NULL, 0)) {
    panic("http_server_listen_and_serve");
  }

  printf("[HTTP-SERVER] Now serving at port %d\n", port);
  
  char in[64];
  while(true) {
    if(scanf("%s", in) < 0) {
      panic("scanf");
    }
    if(strcmp("q", in) == 0) break;
  }

  http_server_free(&server);
  return 0;
}
