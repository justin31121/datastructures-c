#ifndef QUEUE_H_
#define QUEUE_H_

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct{
  void *buffer;
  size_t cap;
  size_t size;
  size_t start;
  size_t memb;
}Queue;

Queue *queue_init(size_t cap, size_t memb);
void queue_append_front(Queue *queue, const void *data);
void queue_append(Queue *queue, const void *data);
void *queue_pop_front(Queue *queue);
void *queue_pop(Queue *queue);
int queue_contains(const Queue *queue, const void *data);
void queue_free(Queue *queue);

#ifdef QUEUE_IMPLEMENTATION

Queue *queue_init(size_t cap, size_t memb) {
  Queue *queue = (Queue *) malloc(sizeof(Queue));
  if(!queue) {
    fprintf(stderr, "ERORR: Can not allocate enough memory: queue_init/queue\n");
    exit(1);    
  }  

  queue->cap = cap;
  queue->size = 0;
  queue->start = 0;
  queue->memb = memb;
  queue->buffer = malloc(queue->cap * queue->memb);
  if(!queue->buffer) {
    fprintf(stderr, "ERORR: Can not allocate enough memory: queue_init/queue->buffer\n");
    exit(1);    
  }  

  return queue;
}

void queue_append_front(Queue *queue, const void *data) {
  if(queue->size>=queue->cap) {
    fprintf(stderr, "ERORR: Queue-Overflow\n");
    exit(1);
  }
  
  int _pos = ((int) queue->start - 1) % (int) queue->cap;
  if(_pos < 0) {
    _pos = (int) queue->cap - 1;
  }
  size_t pos = (size_t) _pos;
  memcpy((char *) queue->buffer + pos*queue->memb, data, queue->memb);
  queue->size++;
  queue->start = pos;
}

void queue_append(Queue *queue, const void *data) {
  if(queue->size>=queue->cap) {
    fprintf(stderr, "ERORR: Queue-Overflow\n");
    exit(1);
  }

  size_t pos = (queue->start + queue->size) % queue->cap;
  memcpy((char *) queue->buffer + pos*queue->memb, data, queue->memb);
  queue->size++;
}

bool queue_maybe_pop(Queue *queue, void **out) {
  if(queue->size<=0) {
    return false;
  }
  
  void *v = (char *) queue->buffer + queue->memb*queue->start;
  queue->start = (queue->start + 1) % queue->cap;
  queue->size--;
  *out = v;

  return true;
}

void *queue_pop(Queue *queue) {
  if(queue->size<=0) {
    fprintf(stderr, "ERORR: Queue-Underflow\n");
    exit(1);
  }

  void *v = (char *) queue->buffer + queue->memb*queue->start;
  queue->start = (queue->start + 1) % queue->cap;
  queue->size--;
  return v;  
}

int queue_contains(const Queue *queue, const void *data) {
  for(size_t i=0;i<queue->size;i++) {
    size_t pos = ((queue->start + i) % queue->cap);
    if(memcmp((char *) queue->buffer + pos*queue->memb, data, queue->memb)==0)
      return (int) i;
  }
  return -1;
}

void queue_free(Queue *queue) {
  if(!queue) return;
  if(queue->buffer) free(queue->buffer);
  free(queue);
}

#endif //QUEUE_IMPLEMENTATION

#endif //QUEUE_H_

