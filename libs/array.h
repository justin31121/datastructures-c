#ifndef ARRAY_H_H
#define ARRAY_H_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct{
  void *data;
  size_t size;
  size_t count;
  size_t msize;
}Arr;

Arr *arr_init(size_t msize);
void arr_push(Arr *arr, void *ptr);
void *arr_get(const Arr *arr, size_t p);
void arr_free(Arr *arr);

#ifdef ARRAY_IMPLEMENTATION

#ifdef ARRAY_NOTNULL

#define ARRAY_CHECK_NOTNULL(arr) do{ \
  if(arr==NULL) {\
  fprintf(stderr, "ERROR: Tried to perform operation on NULL. Expected Arr *.\n");\
  exit(1);\
  }\
}while(0)
#else

#define ARRAY_CHECK_NOTNULL(arr)

#endif 

Arr *arr_init(size_t msize) {
  Arr *arr = (Arr *) malloc(sizeof(Arr));
  if(!arr) {
    fprintf(stderr, "ERROR: Can not allocate enough memory: arr_init\n");
    exit(1);    
  }
  
  arr->msize = msize;
  arr->size = 8;
  arr->count = 0;

  arr->data = malloc(arr->msize * arr->size);
  if(!arr->data) {
    free(arr);
    fprintf(stderr, "ERROR: Can not allocate enough memory: arr_init\n");
    exit(1);
  }

  return arr;
}

void arr_realloc(Arr *arr) {
  ARRAY_CHECK_NOTNULL(arr);
  arr->size *= 2;
  arr->data = realloc(arr->data, arr->msize * arr->size);
  if(!arr->data) {
    fprintf(stderr, "ERROR: Can not allocate enough memory: arr_realloc\n");
    exit(1);
  }
}

void arr_push(Arr *arr, void *ptr) {
  ARRAY_CHECK_NOTNULL(arr);
  if(arr->count>=arr->size) {
    arr_realloc(arr);
  }

  memcpy((char *) arr->data + arr->msize*arr->count, ptr, arr->msize);
  arr->count++;
}

void *arr_get(const Arr *arr, size_t p) {
  ARRAY_CHECK_NOTNULL(arr);
  if(p>=arr->size) {
#ifdef _WIN32
    fprintf(stderr, "ERROR: Index out of Bounce Exception. Requested index: '%lld' but len(array) is '%lld'\n", p, arr->size);
#endif //_WIN32
#ifdef linux
    fprintf(stderr, "ERROR: Index out of Bounce Exception. Requested index: '%ld' but len(array) is '%ld'\n", p, arr->size);
#endif //linu
    exit(1);
  }

  return (char *) arr->data + arr->msize*p;
}

void *arr_pop(Arr *arr) {
  ARRAY_CHECK_NOTNULL(arr);
  if(arr->count==0) {
    fprintf(stderr, "ERROR: Array-Stack-Underflow\n");
    exit(1);    
  }

  void *ret = arr_get(arr, arr->count-1);
  arr->count--;
  return ret;
}

void arr_free(Arr *arr) {
  ARRAY_CHECK_NOTNULL(arr);
  if(arr->data) free(arr->data);
  free(arr);
}

#endif

#endif //ARRAY_H_H
