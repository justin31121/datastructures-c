#ifndef UTIL_H_H
#define UTIL_H_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#define UNIMPLEMENTED(...) do{ fprintf(stderr, "%s:%d: error: %s is not implemented\n", __FILE__, __LINE__, __func__); exit(1);}while(0)
#define UNUSED(...) util_unused(0, __VA_ARGS__);
void util_unused(int n, ...) { (void) n; }

#define not_null(ptr) do{ if(ptr==NULL) {fprintf(stderr, "%s:%d: error: %s: Expected a non NULL pointer\n", __FILE__, __LINE__, __func__); exit(1); }}while(0)
#define panic(cstr) do{ fprintf(stderr, "%s:%d: error: %s: %s\n", __FILE__, __LINE__, __func__, cstr); exit(1);}while(0)
#define warn(cstr) fprintf(stderr, "%s:%d: warning: %s: %s\n", __FILE__, __LINE__, __func__, cstr);


char *slurp_file(const char* file_path, size_t *size);
void write_file(const char *file_path, const char *data);
void write_file_len(const char *file_path, const char *data, size_t size);

#ifdef UTIL_IMPLEMENTATION

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

  res = (char *) malloc((size_t) m + 1);
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
