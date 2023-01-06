#include <stdio.h>

#define HTTP_IMPLEMENTATION
#include "../http.h"

int main() {
  HttpServer *server = http_server_init(6969);

  http_server_listen_and_serve(server, http_server_simple_file_handler, 4);

  char in[64];
  while(true) {
    scanf("%s", in);
    if(strcmp("q", in) == 0) break;
  }
  
  http_server_stop(server);

  http_server_close(server);
  return 0;
}
