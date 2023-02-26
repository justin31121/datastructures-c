#ifndef UTIL_H_H
#define UTIL_H_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>

#define UNIMPLEMENTED(...) do{ fprintf(stderr, "%s:%d: error: %s is not implemented\n", __FILE__, __LINE__, __func__); exit(1);}while(0)
#define UNUSED(...) util_unused(0, __VA_ARGS__);
void util_unused(int n, ...) { (void) n; }

#define not_null(ptr) do{ if(ptr==NULL) {fprintf(stderr, "%s:%d: error: %s: Expected a non NULL pointer\n", __FILE__, __LINE__, __func__); exit(1); }}while(0)
#define panic(...) do{ fprintf(stderr, "%s:%d: error: %s: ", __FILE__, __LINE__, __func__); fprintf(stderr, __VA_ARGS__); if(errno != 0) { fprintf(stderr, ": %s", strerror(errno));} fprintf(stderr, "\n"); exit(1);}while(0)
#define warn(...) do{ fprintf(stderr, "%s:%d: warning: %s:", __FILE__, __LINE__, __func__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }while(0)


void print_bytes(void *_ptr, size_t number_of_bytes);
char *slurp_file(const char* file_path, size_t *size);
bool slurp_file2(const char* file_path, size_t (*write_callback)(const void *data, size_t size, size_t memb, void *userdata), void *userdata);
void write_file(const char *file_path, const char *data);
void write_file_len(const char *file_path, const char *data, size_t size);
bool sendf(bool (*send_callback)(const char *, size_t , void*), void *userdata,
	   char *buffer, size_t buffer_cap, const char *format, ...);

typedef unsigned char u8;
typedef char s8;
typedef unsigned int u32;
typedef int s32;
typedef unsigned long u64;
typedef long s64;

#ifdef UTIL_IMPLEMENTATION

void print_bytes(void *_ptr, size_t number_of_bytes) {
  long ptr = * (long *) _ptr;
  printf("\t");
  for(size_t i=0;i<number_of_bytes;i++) {
    for(int j=0;j<8;j++) {
      printf("%d ", (ptr & (1 << (i*8+j))) > 0);
    }
    if(i!=number_of_bytes-1) printf("| ");
  }
  printf("\n");
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

bool slurp_file2(const char* file_path, size_t (*write_callback)(const void *, size_t, size_t, void *), void *userdata) {
  FILE *f = fopen(file_path, "rb");
  if(!f) {
    fprintf(stderr, "[WARNING]: Can not open file '%s' because: %s\n", file_path, strerror(errno));
    return false;
  }

  size_t buffer_size = 8192 << 2;

  while(true) {
    bool bbreak = false;
    char buffer[buffer_size];
    size_t nbytes = fread(buffer, 1, buffer_size, f);
    if(nbytes != buffer_size) {
      if(ferror(f)) {
	fprintf(stderr, "[WARNING]: Can not read file '%s' because: %s\n", file_path, strerror(errno));
	fclose(f);
	return false;	
      }
      if(feof(f)) {
	bbreak = true;
      }
    }
    if(nbytes > 0) {
      write_callback(buffer, nbytes, 1, userdata);
    }
    if(bbreak) {
      break;
    }
  }

  fclose(f);  
  return true;
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
      if (format[i+1] == 'c') { // %c	
        char c = (char) va_arg(va, int);
	if(!sendf_bar(&context, &buffer_size, &c, 1)) {
	  return false;
	}

	format_last = i+2;
	i++;
      } else if(format[i+1]=='s') { // %	
	const char *argument_cstr = va_arg(va, char *);
	if(!sendf_bar(&context, &buffer_size, argument_cstr, strlen(argument_cstr))) {
	  return false;
	}

	format_last = i+2;
	i++;
      } else if(format[i+1]=='d') { // %d
	int n = va_arg(va, int);

	if(n == 0) {
	  const char *zero = "0";
	  if(!sendf_bar(&context, &buffer_size, zero, strlen(zero))) {
	    return false;
	  }	  
	} else {
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
	}	

	format_last = i+2;
	i++;
      } else if(format[i+1] == '.' && i+3 < format_len &&
		format[i+2] == '*' && format[i+3] == 's') { //%.*s
	
	size_t argument_cstr_len = va_arg(va, size_t);
	const char *argument_cstr = va_arg(va, char *);

	if(!sendf_bar(&context, &buffer_size, argument_cstr, argument_cstr_len)) {
	  return false;
	}

	format_last = i+4;
	i+=3;
      } else if(format[i+1] == '_' && i+3 < format_len &&
		format[i+2] == 'w' && format[i+3] == 's') { //%_ws
	
	size_t argument_cstr_len = va_arg(va, size_t);
	const char *argument_cstr = va_arg(va, char *);
	const char *argument_xormask = va_arg(va, char *); // len 4

	size_t window_cap = 256;
	char window[window_cap];
	size_t window_size = 0;

	for(size_t j=0;j<argument_cstr_len;j++) {
	  window[window_size++] = argument_cstr[j] ^ argument_xormask[j % 4];
	  if(window_size >= window_cap) {
	    if(!sendf_bar(&context, &buffer_size, window, window_size)) {
	      return false;
	    }
	    window_size = 0;
	  }
	}

	if(window_size > 0 && !sendf_bar(&context, &buffer_size, window, window_size)) {
	  return false;
	}

	format_last = i+4;
	i+=3;
      } else {
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
