#include <stdio.h>

#define HTTP_IMPLEMENTATION
#define HTTP_DEBUG
#include "../http.h"

#define JSON_IMPLEMENTATION
#include "../json.h"

Http *http;

void handle_post(string headers, string body, int client) {
  (void) headers;

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
  
  const HttpResponse response = {
    .code=200,
    .message=STRING("OK"),
    .content_type=STRING("text/plain"),
    .body=STRING("{\"key\":\"value\"}")
  };

  
  http_response_send(client, NULL, response);
}

void handle(string method, string path, string headers, string body, int client) {
  if(string_eq(method, STRING("POST")) && string_eq(path, STRING("/post"))) {
    handle_post(headers, body, client);
    return;
  } else if(string_eq(method, STRING("GET"))) {
    http_server_simple_file_handler(method, path, headers, body, client);
    return;
  }

  http_response_send(client, NULL, HTTP_RESPONSE_NOT_FOUND);
}

int main() {
  http = http_init();
  HttpServer *server = http_server_init(6969);

  http_server_listen_and_serve(server, handle, 12);

  char in[64];
  while(true) {
    scanf("%s", in);
    if(strcmp("q", in) == 0) break;
  }
  
  http_server_stop(server);

  http_server_close(server);
  http_close(http);
  return 0;
}
