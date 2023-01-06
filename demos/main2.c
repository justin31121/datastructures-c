#include <stdio.h>

#define HTTP_IMPLEMENTATION
#include "../http.h"

#define STRING_IMPLEMENTATION
#include "../string.h"

#define UTIL_IMPLEMENTATION
#include "../util.h"

#define PORT 6969

#define THREAD_START(threadid_ptr, thread_function, arg) if(pthread_create(threadid_ptr, NULL, thread_function, arg) != 0)  fprintf(stderr, "[WARNING]: Cannot create thread for client\n");

#define THREAD_JOIN(threadid) pthread_join(threadid, NULL);

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
  String_Buffer buffer;
} Client;

#define THREAD_CAP 4

pthread_t threads[THREAD_CAP] = {0};
Client threads_client[THREAD_CAP] = {0};
void (*http_function)(string method, string path, int client) = NULL;

void *handleClient(void *arg) {
  Client *clientStruct = (Client *) arg;
  int client = clientStruct->client;
  bool *used = &(clientStruct->used);
  String_Buffer *buffer = &(clientStruct->buffer);  

  http_make_nonblocking(client);

  fd_set read_fds = {0};
  FD_SET(client, &read_fds);
  
  struct timeval timeout;
  timeout.tv_sec = 15;
  timeout.tv_usec = 0;

  printf("[INFO] Handling client %d in Thread\n", client);

  while(true) {
    buffer->len = 0;
    //printf("[INFO] Selection from client\n");
    HttpAccept select = http_select(client, &read_fds, &timeout); 
    if(select != HTTP_ACCEPT_OK) {
      break;
    }
    
    //printf("[INFO] Succefully selected from client\n");

    if(!http_read(client, NULL, string_buffer_callback, buffer)) {
      //fprintf(stderr, "[WARNING]: Failed to read from client\n");
      break;
    }

    //printf("%s", buffer->data);

    bool methodFound = false;
    string request = string_from(buffer->data, buffer->len);
    while(request.len) {
      string line = string_trim(string_chop_by_delim(&request, '\n'));

      if(!methodFound) {
	string method = string_trim(string_chop_by_delim(&line, ' '));
	(void) method;
	string path = string_trim(string_chop_by_delim(&line, ' '));
	(void) path;
	methodFound = true;
	if(http_function != NULL) {
	  http_function(method, path, client);
	}
	break;
      }
      
      //printf("'"String_Fmt"'\n", String_Arg(line));
    }

    /*
    if(!http_response_send(client, NULL, response)) {
      //fprintf(stderr, "[WARNING]: Failed to write to client\n");
      break;
    }
    */
  }

  printf("[INFO] Closing connection to client %d in Thread\n", client);
  
  close(client);
  
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
  fprintf(stderr, "[WARNING]: Could not find a thread for a client\n");
}

void threads_join() {
  for(int i=0;i<THREAD_CAP;i++) {
    if(threads_client[i].used == false) continue;
    http_close_socket(threads_client[i].client);
    THREAD_JOIN(threads[i]);
    threads_client[i].used = false;
    string_buffer_free(&threads_client[i].buffer);
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
      sleep_ms(100);
    } else if(accept == HTTP_ACCEPT_ERROR) {
      fprintf(stderr, "[WARNING]: Failed to accept client\n");
    } else {
      thread_create(client);
    }
  }
  
  return NULL;
}

void handle_request(string method, string path, int client) { 

  if(!string_eq(method, STRING("GET"))) {
    if(!http_response_send(client, NULL, HTTP_RESPONSE_NOT_FOUND)) {
      fprintf(stderr, "[WARNING] Failed to write to client\n");
    }
    return;
  }

  char file_path[256] = "./index.html";

  bool is_default = string_eq(path, STRING("/"));

  if(!is_default) {
    memcpy(file_path + 1, path.data, path.len);
    file_path[path.len + 1] = 0;
  }

  size_t size;
  char *file_content = slurp_file(file_path, &size);
  if(file_content == NULL) {
    if(!http_response_send(client, NULL, HTTP_RESPONSE_NOT_FOUND)) {
      fprintf(stderr, "[WARNING] Failed to write to client\n");
    }
    return;
  }

  const HttpResponse response = {
    .code=200,
    .message=STRING("OK"),
    .content_type=!is_default
    ? http_content_type_for_filename_len(file_path, path.len+1)
    : HTTP_CONTENT_TYPE_HTML,
    .body=string_from(file_content, size)
  };

  if(!http_response_send(client, NULL, response)) {
    fprintf(stderr, "[WARNING] Failed to write to client\n");
  }
  
  free(file_content);
}

int main() {
  HttpServer *server = http_server_init(PORT);

  if(listen(server->socket, 3) < 0) {
    panic("listen");
  }

  http_function = handle_request;

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
