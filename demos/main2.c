#include <stdio.h>

#define HTTP_IMPLEMENTATION
#include "../http.h"

#define JSON_IMPLEMENTATION
#include "../json.h"

String_Buffer sb = {0};

void handle_post(string headers, string body, int client) {
  (void) headers;

  /*
  Json json;
  if(!json_parse_len(body.data, body.len, &json)) {
    printf("Failed to parse json: '"String_Fmt"'\n", String_Arg(body));

    const HttpResponse response = {
      .code=400,
      .message=STRING("Bad Request"),
      .content_type=STRING("text/plain"),
      .body=STRING("Can not parse json")
    };
    
    http_response_send(client, NULL, response);    
    return;
  }
  json_free_all(json);
  */

  Http *http = http_init();

  if(!http_get(http, "https://www.example.com/", string_buffer_callback, &sb)) {
    warn("http_get failed");
  }

  const HttpResponse response = {
    .code=200,
    .message=STRING("OK"),
    .content_type=STRING("application/json"),
    .body=STRING("{\"key\":\"value\"}")
  };
  
  http_response_send(client, NULL, response);

  printf("%s\n", sb.data);

  http_close(http);
  string_buffer_free(&sb);
}

void handle(string method, string path, string headers, string body, int client) {
  if(string_eq(method, STRING("POST")) && string_eq(path, STRING("/post"))) {
    handle_post(headers, body, client);
  } else if(string_eq(method, STRING("GET"))) {
    http_server_simple_file_handler(method, path, headers, body, client);
  } else {
    http_response_send(client, NULL, HTTP_RESPONSE_NOT_FOUND);
  }
}

#define THREADS_CAP 12
String_Buffer sbs[THREADS_CAP] = {0};

int main() {
  HttpServer *server = http_server_init(HTTP_PORT);

  sb.cap = 1024 * 1000;
  sb.data = malloc(sizeof(char) * sb.cap);

  http_server_listen_and_serve(server, handle, THREADS_CAP);

  char in[64];
  while(true) {
    scanf("%s", in);
    if(strcmp("q", in) == 0) break;
  }
  
  http_server_stop(server);
  http_server_close(server);
  return 0;
}
