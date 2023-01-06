#include <stdio.h>

#define HTTP_IMPLEMENTATION
#include "../http.h"

int main() {
  HttpServer *server = http_server_init(PORT);
  
  //http_server_close(server);  
  printf("ok\n");
  return 0;
}
