#include <stdio.h>

#define ARRAY_IMPLEMENTATION
#include "../array.h"

int main() {
  Arr *arr = arr_init(sizeof(int));

  for(int i=0;i<8;i++) {
    printf("pushing: %d\n", i);
    arr_push(arr, &i);
  }
  
  printf("%lld (%lld)\n", arr->count, arr->size);
  fputc('[', stdout);
  for(size_t i=0;i<arr->count;i++) {
    printf("%d", *(int *) arr_get(arr, i));
    if(i!=arr->count-1) printf(", ");
  }
  fputc(']', stdout);

  arr_free(arr);
  return 0;
}
