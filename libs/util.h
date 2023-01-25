#ifndef UTIL_H_H
#define UTIL_H_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#define UNIMPLEMENTED(...) do{ fprintf(stderr, "%s:%d: error: %s is not implemented\n", __FILE__, __LINE__, __func__); exit(1);}while(0)
#define UNUSED(...) util_unused(0, __VA_ARGS__);
void util_unused(int n, ...) { (void) n; }

#define not_null(ptr) do{ if(ptr==NULL) {fprintf(stderr, "%s:%d: error: %s: Expected a non NULL pointer\n", __FILE__, __LINE__, __func__); exit(1); }}while(0)
#define panic(cstr) do{ fprintf(stderr, "%s:%d: error: %s: %s", __FILE__, __LINE__, __func__, cstr); if(errno != 0) { fprintf(stderr, ": %s", strerror(errno));} fprintf(stderr, "\n"); exit(1);}while(0)
#define warn(cstr) fprintf(stderr, "%s:%d: warning: %s: %s\n", __FILE__, __LINE__, __func__, cstr);


char *slurp_file(const char* file_path, size_t *size);
void write_file(const char *file_path, const char *data);
void write_file_len(const char *file_path, const char *data, size_t size);
bool sendf(bool (*send_callback)(const char *, size_t , void*), void *userdata,
	   char *buffer, size_t buffer_cap, const char *format, ...);


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

/////////////////////////////////////////////////////////////////////////////////

typedef struct{
  bool (*send_callback)(const char *, size_t , void*);
  char *buffer;
  size_t buffer_cap;
  void *userdata;
  bool last;
}Sendf_Context;

static size_t sendf_foo(Sendf_Context *context, size_t buffer_size,
	       const char *cstr, size_t cstr_len, size_t *cstr_off) {
  size_t diff = cstr_len - *cstr_off;

  if(buffer_size + diff < context->buffer_cap) {
    memcpy(context->buffer + buffer_size, cstr + *cstr_off, diff);

    *cstr_off = 0;
    return buffer_size + diff;
  } else{
    size_t buffer_diff = context->buffer_cap - buffer_size;
    memcpy(context->buffer + buffer_size, cstr + *cstr_off, buffer_diff);
    
    (*cstr_off) += buffer_diff;
    return buffer_size + buffer_diff;
  }  
}

static size_t sendf_bar(Sendf_Context *context, size_t *buffer_size, const char *cstr, size_t cstr_len) {
  size_t cstr_off = 0;
  while(true) {
    *buffer_size = sendf_foo(context, *buffer_size, cstr, cstr_len, &cstr_off);
    if(*buffer_size == context->buffer_cap || (context->last && *buffer_size != 0)) {
      if(!context->send_callback(context->buffer, *buffer_size, context->userdata)) {
	return false;
      }
    }
    if(*buffer_size < context->buffer_cap) break;
    *buffer_size = 0;
  }

  return true;
}

bool sendf(bool (*send_callback)(const char *, size_t , void*), void *userdata,
	   char *buffer, size_t buffer_cap, const char *format, ...) {

  Sendf_Context context = {0};
  context.send_callback = send_callback;
  context.buffer = buffer;
  context.buffer_cap = buffer_cap;
  context.userdata = userdata;
  context.last = false;

  va_list va;
  va_start(va, format);
  
  size_t buffer_size = 0;
  size_t format_len = strlen(format);
  size_t format_last = 0;
  for(size_t i=0;i<format_len;i++) {
    if(format[i]=='%' && i+1 < format_len) {      
      if(!sendf_bar(&context, &buffer_size, format + format_last, i - format_last)) {
	return false;
      }
      if(format[i+1]=='s') {
	const char *argument_cstr = va_arg(va, char *);
	if(!sendf_bar(&context, &buffer_size, argument_cstr, strlen(argument_cstr))) {
	  return false;
	}

	format_last = i+2;
	i++;
      } else if(format[i+1]=='d') {
	int n = va_arg(va, int);
	size_t digit_buffer_cap = 32;
	char digit_buffer[digit_buffer_cap];
	size_t digit_buffer_count = 0;
	bool was_negative = false;
	if(n < 0) {
	  was_negative = true;
	  n *= -1;
	}
	while(n > 0) {
	  int m = n % 10;
	  assert(digit_buffer_count < digit_buffer_cap);
	  digit_buffer[digit_buffer_cap - digit_buffer_count++ - 1] = m + '0';
	  n = n / 10;
	}
	if(was_negative) {
	  assert(digit_buffer_count < digit_buffer_cap);
	  digit_buffer[digit_buffer_cap - digit_buffer_count++ - 1] = '-';
	}
	if(!sendf_bar(&context, &buffer_size,
		      digit_buffer + (digit_buffer_cap - digit_buffer_count), digit_buffer_count)) {
	  return false;
	}

	format_last = i+2;
	i++;
      } else if(format[i+1] == '.' && i+3 < format_len &&
		format[i+2] == '*' && format[i+3] == 's'){
	size_t argument_cstr_len = va_arg(va, size_t);
	const char *argument_cstr = va_arg(va, char *);

	if(!sendf_bar(&context, &buffer_size, argument_cstr, argument_cstr_len)) {
	  return false;
	}

	format_last = i+4;
	i+=3;
      }else {
	panic("Unexpected format argument");
      }
    }
  }
  va_end(va);

  context.last = true;
  if(!sendf_bar(&context, &buffer_size, format + format_last, format_len - format_last)) {
    return false;
  }

  return true;
}

#endif //UTIL_IMPLEMENTATION

#endif //UTIL_H_H
