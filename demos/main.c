#include <stdio.h>

#define HTTP_IMPLEMENTATION
#include "../http.h"

#define STRING_IMPLEMENTATION
#include "../string.h"

#define UTIL_IMPLEMENTATION
#include "../util.h"

#define PORT HTTP_PORT

#define THREAD_START(threadid_ptr, thread_function, arg) if(pthread_create(threadid_ptr, NULL, thread_function, arg) != 0)  fprintf(stderr, "[WARNING]: Cannot create thread for client\n");

#define THREAD_JOIN(threadid) pthread_join(threadid, NULL);

const char *response =
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/html\r\n"
  "Content-Length: 12\r\n"
  "Connection: close\r\n"
  "\r\n"
  "Hello, Http!";
ssize_t response_len;

void sleep_ms(int ms) {
  struct timespec sleep_time;
  if(ms < 1000) {
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = ms * 1000000;    
  } else {
    sleep_time.tv_sec = ms / 1000;
    sleep_time.tv_nsec = 0;    
  }
  if(nanosleep(&sleep_time, NULL) == -1) {
    fprintf(stderr, "[WARNING] Failed to sleep\n");
  }
}

void *foo(void *arg) {
  (void) arg;
  struct timeval start, end;
  gettimeofday(&start, NULL);
  
  printf("[INFO] Thread starts to wait\n");
  sleep_ms(5 * 1000);
  gettimeofday(&end, NULL);
  double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
  
  printf("[INFO] Thread has finished waiting (%.6f)\n", elapsed);
  return NULL;
}

typedef struct{
  int client;
  bool used;
} Client;

#define THREAD_CAP 1

pthread_t threads[THREAD_CAP] = {0};
struct timeval threads_lifetime[THREAD_CAP] = {0};
Client threads_client[THREAD_CAP] = {0};

void *handleClient(void *arg) {
  Client *clientStruct = (Client *) arg;
  int client = clientStruct->client;
  bool *used = &(clientStruct->used);
  
  if(!http_read_header(client, NULL, fwrite, stdout)) {
    fprintf(stderr, "[WARNING]: Failed to read from client\n");
  }
  if(!http_send(client, NULL, response)) {
    fprintf(stderr, "[WARNING]: Failed to write to client\n");
  }

  *used = false;

  return NULL;
}

void thread_create(int client) {
  for(int i=0;i<THREAD_CAP;i++) {
    if(threads_client[i].used==true) continue;
    threads_client[i].client = client;
    THREAD_START(&threads[i], handleClient, &threads_client[i]);
    threads_client[i].used = true;
    return;
  }
  fprintf(stderr, "[WARNING]: Could not find a thread for a client");
}

void threads_join() {
  for(int i=0;i<THREAD_CAP;i++) {
    if(threads_client[i].used == false) continue;
    THREAD_JOIN(threads[i]);
    threads_client[i].used = false;
  }
}

typedef struct{
  HttpServer *server;
  bool *stop;
}ServerAndStop;

void *acceptClients(void *arg) {
  ServerAndStop *serverAndStop = (ServerAndStop *) arg;
  HttpServer *server = serverAndStop->server;
  bool *stop = serverAndStop->stop;
  
  while(!(*stop)) {
    int client;
    HttpAccept accept = http_accept(server->socket, &client);
    if(accept == HTTP_ACCEPT_AGAIN) {
      sleep_ms(500);
    } else if(accept == HTTP_ACCEPT_ERROR) {
      fprintf(stderr, "[WARNING]: Failed to accept client\n");
    } else {
      thread_create(client);
    }
  }
  
  return NULL;
}

int main() {
  response_len = strlen(response);
  HttpServer *server = http_server_init(PORT);

  if(listen(server->socket, 3) < 0) {
    panic("listen");
  }

  //START acceptClients
  bool stop = false;
  ServerAndStop serverAndStop = { server, &stop };
  pthread_t acceptClientsThread;
  THREAD_START(&acceptClientsThread, acceptClients, &serverAndStop);
  
  char in[64];
  while(true) {
    scanf("%s", in);
    if(strcmp("q", in) == 0) break;
  }

  //JOIN acceptClients
  stop = true;
  THREAD_JOIN(acceptClientsThread);
  
  threads_join();
  http_server_close(server);  
  return 0;
}
