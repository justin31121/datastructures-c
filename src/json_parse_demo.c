#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "../json.h"

#define FILE_PATH "./prop.json"

#define return_defer(mess) do{			\
    fprintf(stderr, "ERROR: %s\n", mess);	\
    goto defer;					\
  }while(0)

char* slurp_file(const char* file_path, size_t *size) {
  char* content = NULL;
  
  FILE *f = fopen(file_path, "rb");
  if(!f) {
    return_defer("Can not open file");
  }

  if(fseek(f, 0, SEEK_END)<0) {
    return_defer("Can not read file");
  }

  long m = ftell(f);
  if(m<0) {
    return_defer("Can not read file");
  }

  if(fseek(f, 0, SEEK_SET)<0) {
    return_defer("Can not read file");
  }

  content = (char *) malloc((size_t) m+1);
  if(!content) {
    return_defer("Can not allocate enough memory");
  }

  size_t n = fread(content, 1, (size_t) m, f);
  if((int) n!=m) {
    return_defer("Reading failed");
  }

  content[m] = 0;
  if(size) *size = n;

 defer:  
  if(f) fclose(f);
  return content;
}

void write() {
  Json json = json_init_object();
  json_put_null(json, "name");
  json_put_bool(json, "age", true);

  FILE *f = fopen(FILE_PATH, "wb");
  if(!f) {
    fprintf(stderr, "ERORR: Can not open file: '%s'\n", FILE_PATH);
    exit(1);
  }

  json_fprint(f, json);

  fclose(f);

  json_free(json); 
}

int main() {  
  //READ  
  char* content = slurp_file("sample.json", NULL);

  Json json;
  if(json_parse(content, &json)) {
    //json_fprint(stdout, json);
  }
  else {
    printf("Parsing Failed\n");
    return 0;
  }

  json_fprint(stdout, json_opt(json_get_array(json_get_object(json_get_object(json_get_object(json, "quiz"), "maths"), "q1"), "options"), 1));
  
  if(content) free(content);
  json_free_all(json);

  return 0;
}
