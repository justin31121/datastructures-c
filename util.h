#ifndef UTIL_H_H
#define UTIL_H_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#define UNIMPLEMENTED() do{ fprintf(stderr, "%s:%d:ERROR: %s is not implemented\n", __FILE__, __LINE__, __func__); exit(1);}while(0)

/*
#define UNIMPLEMENTED_2(_num_args, ...) do{				
  int num_args = _num_args;						
  va_list args;								
  void *p = NULL;							
  va_start(args, num_args);						
  while(num_args--) { p = (void *) va_arg(args, void*);	}	    
va_end(args);							
(void) p;							
  }while(0)


							
*/


void warn(const char* message);
void panic(const char* message);

char *slurp_file(const char* file_path, size_t *size);
void write_file(const char *file_path, const char *data);
void write_file_len(const char *file_path, const char *data, size_t size);

#ifdef UTIL_IMPLEMENTATION

void warn(const char* message) {
  fprintf(stderr, "WARNING: %s\n", message);
}

void panic(const char* message) {
  fprintf(stderr, "ERORR: %s\n", message);
  exit(1);
}

void write_file(const char *file_path, const char *data) {
  write_file_len(file_path, data, strlen(data));
}

void write_file_len(const char *file_path, const char *data, size_t size) {
  FILE *f = fopen(file_path, "wb");
  if(!f) {
    fprintf(stderr, "ERORR: Can not open file '%s' because: %s\n", file_path, strerror(errno));
    exit(1);
  }

  fwrite(data, size, 1, f);
  if(ferror(f)) {
    fprintf(stderr, "Could not write to file '%s' because: %s\n", file_path, strerror(errno));
    exit(1);
  }

  fclose(f);
}

char *slurp_file(const char* file_path, size_t *size) {
  char *res = NULL;
  
  FILE *f = fopen(file_path, "rb");
  if(!f) {
    fprintf(stderr, "[WARNING]: Can not open file '%s' because: %s\n", file_path, strerror(errno));
    return NULL;
  }

  if(fseek(f, 0, SEEK_END) < 0) {
    fprintf(stderr, "[WARNING]: Can not read file '%s' because: %s\n", file_path, strerror(errno));
    fclose(f);
    return NULL;
  }

  long m = ftell(f);
  if(m < 0) {
    fprintf(stderr, "[WARNING]: Can not read file '%s' because: %s\n", file_path, strerror(errno));
    fclose(f);
    return NULL;
  }  

  if(fseek(f, 0, SEEK_SET) < 0) {
    fprintf(stderr, "[WARNING]: Can not read file '%s' because: %s\n", file_path, strerror(errno));
    fclose(f);
    return NULL;
  }

  res = (char *) malloc(m + 1);
  if(!res) {
    fprintf(stderr, "[WARNING]: Can not allocate enough memory for file '%s'\n", file_path);
    fclose(f);
    return NULL;
  }

  size_t _m = (size_t) m;
  size_t n = fread(res, 1, _m, f);
  if(n != _m) {
    fprintf(stderr, "[WARNING]: Can not read file '%s' because: %s\n", file_path, strerror(errno));
    fclose(f);
    exit(1);    
  }
  res[n] = 0;

  if(size) *size = n;

  fclose(f);
  return res;
}

#endif //UTIL_IMPLEMENTATION

#endif //UTIL_H_H
