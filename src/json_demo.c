#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "../libs/json.h"

int main() { 
  Json small = json_init_object();
  json_put_string(small, "ab", "cd");
  
  Json arr = json_init_array();
  for(int i=0;i<3;i++) {
    json_push_object(arr, &small);
  }
  
  Json json = json_init_object();

  json_put_null(json, "nullValue");
  json_put_bool(json, "trueOrFalse", true);
  json_put_string(json, "name", "bar");
  json_put_int(json, "foo", 59);  
  json_put_array(json, "array", &arr);
  
  json_fprint(stdout, json);

  //=========================================

  json_remove(json, "array");
  fputc('\n', stdout);
  json_fprint(stdout, json);
  printf("\n\n");

  json_free(arr);
  json_free(small);
  json_free(json);

  //=========================================

  arr = json_init_array();

  for(int i=0;i<10;i++) {
    json_push_int(arr, i*2);
  }

  for(int i=0;i<json_size(arr);i++) {
    printf("%d, ", json_opt_int(arr, i));
  }
  printf("\n\n");
  
  json_free(arr);

  //=====================================

  json = json_init_object();
  json_put_int(json, "bazz", 3);
  
  printf("%s\n", json_has(json, "bar") ? "true" : "false");

  //json_get(json, "bar");
  //throws error, since "bar" is not in json

  printf("%d\n", json_get_int(json, "bazz"));

  json_free(json);
  
  return 0;
}
