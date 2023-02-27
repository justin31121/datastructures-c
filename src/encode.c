#include <stdio.h>

#define HTTP_IMPLEMENTATION
#define HTTP_NO_SSL
#include "../libs/http.h"

int main() {

  const char* s = "Pussy & Millions";
  char buffer[2][1024];

  printf("%s\n", s);

  size_t size;
  if(!http_encodeURI(s, strlen(s), buffer[0], 1024, &size)) {
    panic("http_encodeURI");
  }

  printf("%.*s\n", (int) size, buffer[0]);

  size_t size2;
  if(!http_decodeURI(buffer[0], size, buffer[1], 1024, &size2)) {
    panic("http_encodeURI");
  }

  printf("%.*s\n", (int) size2, buffer[1]);

  return 0;
}
