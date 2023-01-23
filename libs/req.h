#ifndef REQ_H
#define REQ_H

#include <stdbool.h>
#include <curl/curl.h>// -lcurl

typedef CURL* Req;

typedef struct{
  char *target;
  size_t cap;
  size_t size;  
}Req_Context;

Req req_init();
bool req_get(const Req req, const char* url, char *buffer, size_t buffer_size, size_t *size);
void req_close(Req req);

#ifdef REQ_IMPLEMENTATION

Req req_init() {
  return curl_easy_init();
}

size_t req_h_write_callback(char* data, size_t size, size_t nmemb, void *userp) {
  Req_Context *context = (Req_Context *) userp;
  
  if(context->target == NULL) return 0;
  size_t ssize = size * nmemb;
  if(context->size + ssize >= context->cap - 1) {
    printf("INFO: Overflow\n");
    return 0;
  }
  memcpy(context->target + context->size, data, ssize);
  context->size += ssize;
  return ssize;
}

bool req_get(const Req req, const char* url, char *buffer, size_t buffer_size, size_t *size) {
  Req_Context context = {
    buffer, buffer_size, 0
  };

  curl_easy_setopt(req, CURLOPT_URL, url);
  curl_easy_setopt(req, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(req, CURLOPT_WRITEFUNCTION, req_h_write_callback);
  curl_easy_setopt(req, CURLOPT_WRITEDATA, &context);

  CURLcode res = curl_easy_perform(req);
  if(res!=CURLE_OK) {
    return false;
  }

  context.target[context.size] = 0;
  if(size) *size = context.size;
  
  return true;
}

void req_close(Req req) {
  if(req!=NULL) curl_easy_cleanup(req);
}

#endif //REQ_IMPLEMENTATION

#endif //REQ_H
