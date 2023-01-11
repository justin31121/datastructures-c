#include <stdio.h>
#include <time.h>

#define HTTP_IMPLEMENTATION
#include "../http.h"

#define UTIL_IMPLEMENTATION
#include "../util.h"

int main(int argc, char **argv) {

  char *http_method = "GET";

  if(argc < 2) {
    printf("USAGE: main <url>\n");
    panic("Please provide enough arguments");
  }
  
  if(argc > 2) {
    http_method = argv[2];
  }

  Http http;
  if(!http_init(&http)) {
    panic("http_init");
  }

  String_Buffer sb = {0};
  string_buffer_reserve(&sb, 1024 * 10000);
  
  clock_t t = clock();
  if(!http_request(&http, argv[1], http_method, NULL, NULL, string_buffer_callback, &sb)) {
    panic("http_request");
  }
  t = clock() - t;

  if(sb.len > 0 && sb.len < 64000) printf("%s\n\n", sb.data);
  printf("%lld bytes\n\n", sb.len);
  string_buffer_free(&sb);

  http_free(&http);

  printf("It took %.6f ms\n", 1000 * ((double) t)/CLOCKS_PER_SEC);
  
  return 0;
}
