#include <stdio.h>

#define HTTP_IMPLEMENTATION
#define HTTP_NO_SSL
#include "../libs/http.h"

static char cwd[PATH_MAX];

void handle(const HttpRequest *request, Http *client, void *arg) {
  (void) arg;

  if(string_eq(request->method, STRING("GET"))) {
    http_send_files(client, cwd, "/index.html", request->route);
    return;
  }

  http_send_not_found(client);

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

  if(!http_server_listen_and_serve(&server, http_server_simple_file_handler, 24, NULL, 0)) {
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