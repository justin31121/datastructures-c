#include <stdio.h>

#define HTTP_IMPLEMENTATION
#define HTTP_NO_SSL
#include "../libs/http.h"

#define WATCHER_IMPLEMENTATION
#include "../libs/watcher.h"

static char cwd[PATH_MAX];

void handle(const HttpRequest *request, Http *client, void *arg) {
  (void) arg;
  
  if(string_eq(request->method, STRING("GET"))) {
    http_send_files(client, "./", "/index.html", request->route);
    return;
  }

  http_send_not_found(client);
}

bool queuedChange = false;
pthread_mutex_t lock;

void handle_websocket(const char *message, size_t message_len, Http *client, void *arg) {
  (void) message;
  (void) message_len;
  
  const char *pong = "pong";
  size_t pong_len = strlen(pong);
  const char *reload = "reload";
  size_t reload_len = strlen(reload);
  
  HttpServer *server = (HttpServer *) arg;

  for(;server->running;) {
    http_sleep_ms(100);
    if(queuedChange) {
      http_websocket_send_len(reload, reload_len, client);
      
      pthread_mutex_lock(&lock);
      queuedChange = false;
      pthread_mutex_unlock(&lock);

      return;
    } else {
      http_websocket_send_len(pong, pong_len, client);
    }    
  }
}

void handle_watcher_event(Watcher_Event event, const char *name) {
  (void) event;
  (void) name;
  
  pthread_mutex_lock(&lock);
  queuedChange = true;
  pthread_mutex_unlock(&lock);
}

int main(int argc, char **argv) {
  int port = 6060;

  if(argc > 1) {
    string arg = string_from_cstr(argv[1]);
    if(!string_chop_int(&arg, &port)) {
      fprintf(stderr, "ERROR:\tPlease provide a valid port, or use the default one.\n");
      fprintf(stderr, "USAGE:\thttpserver [<port>]\n");
      exit(1);
    }
  }

  if(pthread_mutex_init(&lock, NULL) != 0) {
    panic("pthread_mutex_init");
  }
  
  HttpServer server;
  if(!http_server_init(&server, port, NULL, NULL)) {
    panic("http_server_init");
  }
  
#ifdef _WIN32
  #include <direct.h>
  if(!_getcwd(cwd, sizeof(cwd))) {
    panic("getcwd");
  }
#endif //_WIN32
#ifdef linux
  if(!getcwd(cwd, sizeof(cwd))) {
    panic("getcwd");
  }
#endif //linux

  server.handle_websocket = handle_websocket;
  if(!http_server_listen_and_serve(&server, handle, 24, &server, 0)) {
    panic("http_server_listen_and_serve");
  }

  printf("[HTTP-SERVER] Now serving at port %d\n", port);

  Watcher watcher;
  if(!watcher_init(&watcher, "./", handle_watcher_event)) {
    panic("watcher_init");
  }

  if(!watcher_start(&watcher)) {
    panic("watcher_start");
  }

  char in[64];
  while(true) {
    if(scanf("%s", in) < 0) {
      panic("scanf");
    }
    if(strcmp("q", in) == 0) break;
  }

  //watcher_free(&watcher);
  //http_server_free(&server);
  //pthread_mutex_destroy(&lock);
  return 0;
}
