#ifndef DOWNLOAD_H_H
#define DOWNLOAD_H_H

#define HTTP_IMPLEMENTATION
#include <libs/http.h>

#define UTIL_IMPLEMENTATION
#include <libs/util.h>

#define NUMBER_OF_THREADS (64)

typedef struct{
  Thread id;
  u8 *memory;
  size_t memory_size;
  const char *url;
  int start;
  int end;
}_Thread;

size_t download_thread_callback(const void *data, size_t size, size_t memb, void *userdata);
void *download_thread_function(void *arg);
bool download_get_size(const char *url, size_t *data_size);
bool download2(const char *url, u8 *buffer, size_t buffer_size, size_t *data_size);
bool download(const char *url, u8 *buffer, size_t download_size);

#ifdef DOWNLOAD_IMPLEMENTATION

size_t download_thread_callback(const void *data, size_t size, size_t memb, void *userdata) {
  (void) memb;
  _Thread *thread = (_Thread *) userdata;
  memcpy(thread->memory + thread->memory_size, data, size);
  thread->memory_size += size;
  return size;
}

void *download_thread_function(void *arg) {
  _Thread *thread = (_Thread *) arg;

  char buffer[256];
  if(snprintf(buffer, 256, "Range: bytes=%d-%d\r\n", thread->start, thread->end) >= 256) {
    panic("buffer overflow in thread_function");
  }

  if(!http_get(thread->url, download_thread_callback, thread, NULL, buffer)) {
    panic("http_get");
  }

  return NULL;
}

bool download_get_size(const char *url, size_t *data_size) {
  HttpHeader header;
  if(!http_head(url, &header, NULL)) {
    return false;
  }

  string key = STRING("Content-Length");
  string key2 = STRING("content-length");
  string value;
  if(!http_header_has(&header, key, &value) && !http_header_has(&header, key2, &value)) {
    return false;
  }

  int content_length;
  if(!string_chop_int(&value, &content_length)) {
    return false;
  }

  *data_size = (size_t) content_length;
  return true;
}

bool download(const char *url, u8 *buffer, size_t download_size) {
  s32 number_of_threads = NUMBER_OF_THREADS;
  s32 thread_portion = download_size / number_of_threads;

  _Thread threads[NUMBER_OF_THREADS] = {0};


  bool result = true;
  for(s32 i=0;i<number_of_threads;i++) {
    threads[i].url = url;

    threads[i].start = thread_portion * i;
    threads[i].end = thread_portion * (i+1) - 1;
    if(i == NUMBER_OF_THREADS - 1) {
      threads[i].end = download_size;
    }

    threads[i].memory = buffer + threads[i].start;
    threads[i].memory_size = 0;

    if(!thread_create(&threads[i].id, download_thread_function, &threads[i])) {
      result = false;
      break;
    }
  }

  for(s32 i=0;i<number_of_threads;i++) {
    thread_join(threads[i].id);
  }

  return result;
}

bool download2(const char *url, u8 *buffer, size_t buffer_size, size_t *data_size) {

  size_t content_length;
  if(!download_get_size(url, &content_length)) {
    return false;
  }

  if(buffer_size < content_length) {
    return false;
  }

  *data_size = content_length;
  
  return download(url, buffer, content_length);
}

#endif //DOWNLOAD_IMPLEMENTATION

#endif //DOWNLOAD_H_H
