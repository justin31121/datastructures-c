#define HTTP_IMPLEMENTATION
#include "../http.h"

#define THREADS_CAP 128

typedef struct{
  String_Buffer sb;
  String_Buffer res;
}Context;

void handle(const HttpRequest *request, Http *client, void *arg) {
  
  if(string_eq(request->method, STRING("GET"))) {
    http_server_simple_file_handler(request, client, arg);
    return;
  } if(!string_eq(request->method, STRING("POST")) || !string_eq(request->route, STRING("/post"))) {
    http_send_http_response(client, NULL, NULL, 0);
    return;
  }

  Context *context = (Context *) arg;
  String_Buffer *sb = &(context->sb);
  sb->len = 0;
  
  String_Buffer *res = &(context->res);
  res->len = 0;
  
  string_buffer_append(res, request->body.data, request->body.len);
  string_buffer_append(res, "\0", 1);

  printf("%s\n", res->data);

  bool proxy_req = http_get(NULL, res->data, string_buffer_callback, sb);
  if(!proxy_req || sb->len == 0) {
    HttpResponse response = {0};
    response.code = 500;
    response.message = STRING("Internal Server Error");
    response.body = STRING("Can not reach hostname");
    response.content_type = STRING("text/plain");
    http_send_http_response(client, &response, NULL, 0);
    return;
  }
  
  string_buffer_reserve(res, 2*sb->cap);

  HttpResponse response = {0};
  response.code = 200;
  response.message = STRING("OK");
  response.body = string_from(sb->data, sb->len);
  response.content_type = STRING("text/plain");
  http_send_http_response(client, &response, res->data, res->cap);
}

Context sbs[THREADS_CAP] = {0};

int main() {  
  HttpServer server;
  //if(!http_server_init(&server, HTTP_PORT, NULL, NULL)) {
  if(!http_server_init(&server, 6969, "cert.pem", "key.pem")) {
    panic("http_server_init");
  }
  
  if(!http_server_listen_and_serve(&server, handle, THREADS_CAP, sbs, sizeof(sbs[0]))) {
    panic("http_server_listen_and_serve");
  }
  
  char in[64];
  while(true) {
    scanf("%s", in);
    if(strcmp("q", in) == 0) break;
  }
  
  http_server_free(&server);

  for(int i=0;i<THREADS_CAP;i++) {
    string_buffer_free(&sbs[i].sb);
    string_buffer_free(&sbs[i].res);
  }
  return 0;
}
