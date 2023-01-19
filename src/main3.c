#include <stdio.h>
#include <time.h>

#define HTTP_IMPLEMENTATION
#define HTTP_DEBUG
#include "../libs/http.h"

#define HOST "www.google.com"

int main() {
  Http http;
  if(!http_init(&http, HOST, true)) {
    panic("http_init");
  }

  String_Buffer sb = {0};
  string_buffer_reserve(&sb, 10 * 1024);
  
  clock_t t = clock();
  if(!http_request(&http, "/", "GET", NULL, NULL,
		   string_buffer_callback, &sb)) {
    panic("http_request");
  }
  t = clock() - t;

  printf("/////////////////////////////////////////\n");
  printf("%s", sb.data);
  printf("/////////////////////////////////////////\n");
  
  printf("Read %lld bytes\n", sb.len);
  double time_taken = ((double)t)/CLOCKS_PER_SEC;
  printf("\tRequest took %d ms\n", (int) (time_taken * 1000));

  string_buffer_free(&sb);
  http_free(&http);
  
  return 0;
}
