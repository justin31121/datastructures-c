#ifndef ARRAY_H_H
#define ARRAY_H_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ARRAY_DEF
#define ARRAY_DEF static inline
#endif //ARRAY_DEF

typedef struct{
  void *data;
  size_t size;
  size_t count;
  size_t msize;
}Arr;

ARRAY_DEF Arr *arr_init(size_t msize);
ARRAY_DEF Arr *arr_init2(size_t msize, size_t n);

ARRAY_DEF void *arr_push(Arr *arr, void *ptr);
ARRAY_DEF void *arr_pop(Arr *arr);
ARRAY_DEF void *arr_get(const Arr *arr, size_t p);

ARRAY_DEF int arr_contains(const Arr *arr, void *src);
ARRAY_DEF int arr_index(const Arr *arr, void *src);
ARRAY_DEF void arr_free(Arr *arr);

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

ARRAY_DEF Arr *arr_init(size_t msize) {
  return arr_init2(msize, 8);
}

ARRAY_DEF Arr *arr_init2(size_t msize, size_t n) {
  Arr *arr = (Arr *) malloc(sizeof(Arr));
  if(!arr) {
    fprintf(stderr, "ERROR: Can not allocate enough memory: arr_init\n");
    exit(1);    
  }
  
  arr->msize = msize;
  arr->size = n;
  arr->count = 0;

  arr->data = malloc(arr->msize * arr->size);
  if(!arr->data) {
    free(arr);
    fprintf(stderr, "ERROR: Can not allocate enough memory: arr_init\n");
    exit(1);
  }

  return arr;  
}

ARRAY_DEF void arr_realloc(Arr *arr) {
  ARRAY_CHECK_NOTNULL(arr);
  arr->size *= 2;
  arr->data = realloc(arr->data, arr->msize * arr->size);
  if(!arr->data) {
    fprintf(stderr, "ERROR: Can not allocate enough memory: arr_realloc\n");
    exit(1);
  }
}

ARRAY_DEF void *arr_push(Arr *arr, void *ptr) {
  ARRAY_CHECK_NOTNULL(arr);
  if(arr->count>=arr->size) {
    arr_realloc(arr);
  }
  
  void *dest = (char *) arr->data + arr->msize*arr->count;
  memcpy(dest, ptr, arr->msize);
  arr->count++;
  return dest;
}

ARRAY_DEF void *arr_get(const Arr *arr, size_t p) {
  ARRAY_CHECK_NOTNULL(arr);
  if(p>=arr->size) {
      fprintf(stderr, "ERROR: Index out of Bounce Exception. Requested index: '%d' but len(array) is '%d'\n", (int) p, (int) arr->size);
    exit(1);
  }

  return (char *) arr->data + arr->msize*p;
}

ARRAY_DEF int arr_contains(const Arr *arr, void *src) {
    ARRAY_CHECK_NOTNULL(arr);
  void *data = arr->data;

  for(size_t i=0;i<arr->count;i++) {
    if(memcmp(data, src, arr->msize) == 0) {
      return 1;
    }
  }

  return 0;
}

ARRAY_DEF int arr_index(const Arr *arr, void *src) {
    ARRAY_CHECK_NOTNULL(arr);
    return (int) (((char *) src - (char *) arr->data) / arr->msize);
}

ARRAY_DEF void *arr_pop(Arr *arr) {
  ARRAY_CHECK_NOTNULL(arr);
  if(arr->count==0) {
    fprintf(stderr, "ERROR: Array-Stack-Underflow\n");
    exit(1);    
  }

  void *ret = arr_get(arr, arr->count-1);
  arr->count--;
  return ret;
}

ARRAY_DEF void arr_free(Arr *arr) {
  ARRAY_CHECK_NOTNULL(arr);
  if(arr->data) free(arr->data);
  free(arr);
}

#endif

#endif //ARRAY_H_H
