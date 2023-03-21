#include <stdio.h>

#define HASHTABLE_IMPLEMENTATION
#include "../libs/hashtable.h"

int main() {
  Ht *ht = ht_init();

  float f = 3.1415f;
  ht_insert(ht, "foo", &f, sizeof(float));

  //ht_remove(ht, "foo");

  if(ht_has(ht, "foo"))
    printf("%f\n", *(float *) ht_get(ht, "foo"));
  else
    printf("There is no value\n");

  ht_free(ht);
  return 0;
}
