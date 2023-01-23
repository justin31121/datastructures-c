#define HTTP_IMPLEMENTATION
#include "../libs/http.h"

#define THREADS_CAP 128

typedef struct{
  String_Buffer sb;
  String_Buffer res;
}Context;

void handle(const HttpRequest *request, Http *client, void *arg) {
  if(string_eq(request->method, STRING("GET"))) {
    http_send_files(client, "rsc", "/index.html", request->route);
    return;
  } else if(string_eq(request->method, STRING("POST"))) {
    Context *context = (Context *) arg;
    String_Buffer *sb = &(context->sb);
    sb->len = 0;
    String_Buffer *res = &(context->res);
    res->len = 0;
  
    string_buffer_append(res, request->body.data, request->body.len);
    string_buffer_append(res, "\0", 1);

    bool proxy_req = http_get(res->data, string_buffer_callback, sb);
    if(!proxy_req || sb->len == 0) {
      const char *message = "Can not reach hostname";
      http_respond(client, HTTP_STATUS_INTERNAL_ERROR, "text/plain", message, strlen(message));
      return;
    }    
    
    http_respond(client, HTTP_STATUS_OK, "application/json", sb->data, sb->len);
    return;
  }

  http_send_not_found(client);
}

Context sbs[THREADS_CAP] = {0};

int main() {  
  HttpServer server;
  if(!http_server_init(&server, HTTP_PORT, NULL, NULL)) {
  //if(!http_server_init(&server, 6969, "./rsc/cert.pem", "./rsc/key.pem")) {
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
