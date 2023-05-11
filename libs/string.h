#ifndef STRING_H
#define STRING_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#ifdef STRING_IMPLEMENTATION
#define UTIL_IMPLEMENTATION
#endif //STRING_IMPLEMENTATION

#include "util.h"

typedef struct {
  size_t len;
  const char* data;
} string;

#define STRING_STATIC(cstr_lit) {.len=sizeof(cstr_lit), .data=cstr_lit}
#define STRING(cstr_lit) string_from(cstr_lit, sizeof(cstr_lit) -1)
#define String_Fmt "%.*s"
#define String_Arg(s) ((int) (s).len), (s).data

string string_from(const char* data, size_t count);
string string_from_cstr(const char* data);
string string_trim_left(string s);
string string_trim_right(string s);
string string_trim(string s);
string string_chop_by_delim(string *s, char delim);
string string_chop_left(string *s, size_t n);
string string_chop_right(string *s, size_t n);
bool string_chop_string(string *s, string dst);
bool string_chop_int(string *s, int *n);
bool string_chop_hex(string *s, uint64_t *n);
bool string_chop_int64_t(string *s, int64_t *n);

int string_index_of(string s, const char *cstr);
int string_index_of_offset(string s, const char *cstr, size_t offset);

int string_index_of2(string s, string t);
int string_index_of_offset2(string s, string t, size_t offset);

string string_substring(string s, size_t start, size_t end);
bool string_eq(string s, string t);

char* string_to_cstr(string *s);

bool cstr_contains(const char *cstr, size_t cstr_size, const char *val, size_t val_size);
int cstr_index_of(const char* cstr, size_t cstr_size, const char *val, size_t val_size);

//STRING_BUFFER

typedef struct{
  char *data;
  size_t len;
  size_t cap;
}String_Buffer;

bool string_buffer_append(String_Buffer *sb, const char *data, size_t data_size);
bool string_buffer_append_string(String_Buffer *sb, string s);
bool string_buffer_reserve(String_Buffer *sb, size_t data_size);

size_t string_buffer_callback(const void *data, size_t size, size_t memb, void *userdata);

void string_buffer_free(String_Buffer *sb);

const char *tprintf(String_Buffer *sb, const char *format, ...);
string tsprintf(String_Buffer *sb, const char *format, ...);

#ifdef STRING_IMPLEMENTATION

bool cstr_contains(const char *cstr, size_t cstr_size, const char *val, size_t val_size) {
  return cstr_index_of(cstr, cstr_size, val, val_size) >= 0;
}

int cstr_index_of(const char* cstr, size_t cstr_size, const char *val, size_t val_size) {
  if(val_size > cstr_size) {
    return -1;
  }
  for(size_t i=0;i<=cstr_size - val_size;i++) {
    int found = 1;
    for(size_t j=0;j<val_size;j++) {
      if(cstr[i+j] != val[j]) {
	found = 0;
	break;
      }
    }
    if(found) {
      return (int) i;
    }
  }
  return -1;
}

bool string_chop_int64_t(string *s, int64_t *n) {
  size_t i=0;
  int64_t sum = 0;
  int negative = 0;
  if(s->len && s->data[0]=='-') {
    negative = 1;
    i++;
  }
  while(i<s->len && '0' <= s->data[i] && s->data[i] <= '9') {
    sum*=10;
    int digit = (s->data[i] - '0');
    sum+=digit;
    i++;
  }

  s->data+=i;
  s->len-=i;

  if(negative) sum*=-1;
  if(n) *n = sum;

  return i>0;
}

bool string_chop_int(string *s, int *n) {
  size_t i=0;
  int sum = 0;
  int negative = 0;
  if(s->len && s->data[0]=='-') {
    negative = 1;
    i++;
  }
  while(i<s->len && '0' <= s->data[i] && s->data[i] <= '9') {
    sum*=10;
    int digit = (s->data[i] - '0');
    sum+=digit;
    i++;
  }

  s->data+=i;
  s->len-=i;

  if(negative) sum*=-1;
  if(n) *n = sum;

  return i>0;
}

bool string_chop_hex(string *s, uint64_t *n) {
  size_t i=0;
  uint64_t sum = 0;

  while(true) {
    if(i>=s->len) break;
    bool isDigit = '0' <= s->data[i] && s->data[i] <= '9';
    bool isAlphaSmall = 'a' <= s->data[i] && s->data[i] <= 'f';
    bool isAlpha = 'A' <= s->data[i] && s->data[i] <= 'F';

    if(isDigit) {
      sum*=16;
      int digit = (s->data[i] - '0');
      sum+=digit;
    } else if(isAlphaSmall) {
      sum*=16;
      int digit = (s->data[i] - 'W');
      sum+=digit;
    } else if(isAlpha) {
      sum*=16;
      int digit = (s->data[i] - '7');
      sum+=digit;
    } else {
      break;
    }

    i++;
  }

  s->data+=i;
  s->len-=i;

  if(n) *n = sum;
  
  return i>0;
}

string string_from(const char* data, size_t len) {
  string s;
  s.len = len;
  s.data = data;
  return s;
}

string string_from_cstr(const char* data) {
  return string_from(data, strlen(data));
}

string string_trim_left(string s) {
  size_t i = 0;
  while(i<s.len && isspace(s.data[i])) {
    i++;
  }
  return string_from(s.data+i, s.len-i);
}

string string_trim_right(string s) {
  size_t i = 0;
  while(i<s.len && isspace(s.data[s.len - 1 - i])) {
    i++;
  }
  return string_from(s.data, s.len-i);
}

string string_trim(string s) {
  return string_trim_right(string_trim_left(s));
}

int string_index_of(string s, const char *cstr) {
  return cstr_index_of(s.data, s.len, cstr, strlen(cstr));
}

int string_index_of_offset(string s, const char *cstr, size_t offset) {
  return cstr_index_of(s.data + offset, s.len - offset, cstr, strlen(cstr)) + offset;
}

int string_index_of2(string s, string t) {
  return cstr_index_of(s.data, s.len, t.data, t.len);
}

int string_index_of_offset2(string s, string t, size_t offset) {
  return cstr_index_of(s.data + offset, s.len - offset, t.data, t.len) + offset;
}

string string_substring(string s, size_t start, size_t end) {
  if(start >= s.len || end > s.len) {
    return (string) {0};
  }
  return (string) {.data = s.data + start, .len = end - start};
}

bool string_eq(string s, string t) {
  if(s.len != t.len) {
    return false;
  }

  for(size_t i=0;i<s.len;i++) {
    if(s.data[i] != t.data[i]) {
      return false;
    }
  }
  return true;
}

void string_in_cstr(string s, char* target) {
  for(size_t i=0;i<s.len;i++) {
    target[i]=s.data[i];
  }

  target[s.len]='\0';
}

char* string_to_cstr(string *s) {
  char* res = (char *) malloc( s->len * sizeof(char) );
  
  for(size_t i=0;i<s->len;i++) {
    res[i]=s->data[i];
  }
  res[s->len]='\0';
  return res;
}

string string_chop_by_delim(string *s, char delim) {
  size_t i = 0;
  while(i < s->len && s->data[i]!=delim) {
    i+=1;
  }
  
  string result = string_from(s->data, i);
  
  if(i < s->len) {
    s->len -= i+1;
    s->data += i+1;
  }
  else {
    s->len -= i;
    s->data += i;
  }

  return result;
}

string string_chop_left(string *s, size_t n) {
  if(n > s->len) {
    n = s->len;
  }

  string result = string_from(s->data, n);

  s->data += n;
  s->len -= n;

  return result;
}

string string_chop_right(string *s, size_t n) {
  if(n > s->len) {
    n = s->len;
  }

  string result = string_from(s->data - (s->len + n), s->len - n);

  s->len -= n;

  return result;
}

bool string_chop_string(string *s, string dst) {
  int pos = string_index_of2(*s, dst);
  if(pos != 0) {
    return false;
  }
  string_chop_left(s, dst.len);
  return true;
}

size_t string_buffer_callback(const void *data, size_t size, size_t memb, void *userdata) {
  return string_buffer_append((String_Buffer *) userdata, data, size*memb) ? size : 0;
}

bool string_buffer_send_callback(const char *data, size_t data_size, void *_sb) {
  String_Buffer *sb = (String_Buffer *) _sb;
  return string_buffer_append(sb, data, data_size);
}

bool string_buffer_append(String_Buffer *sb, const char *data, size_t data_size) {
  if(!sb) {
    return false;
  }

  if(data_size == 0) {
    return true;
  }

  size_t new_cap = sb->cap == 0 ? 64 : sb->cap;
  while(sb->len + data_size >= new_cap) new_cap *=2;
  if(new_cap != sb->cap) {
    sb->cap = new_cap;
    sb->data = (char *) realloc(sb->data, sb->cap);
    if(!sb->data) {
      return false;
    }
  }
  memcpy(sb->data + sb->len, data, data_size);
  sb->len += data_size;
  return true;
}

bool string_buffer_reserve(String_Buffer *sb, size_t data_size) {
  if(!sb) {
    return false;
  }

  if(data_size == 0) {
    return true;
  }

  if(sb->cap >= data_size) {
    return true;
  }

  size_t new_cap = sb->cap == 0 ? 64 : sb->cap;
  while(data_size >= new_cap) new_cap*=2;
  sb->cap = new_cap;
  sb->data = (char *) realloc(sb->data, sb->cap);
  if(!sb->data) {
    return false;
  }

  return true;
}

bool string_buffer_append_string(String_Buffer *sb, string s) {
  return string_buffer_append(sb, s.data, s.len);
}

void string_buffer_clear(String_Buffer *sb) {
  if(!sb) return;
  sb->data = NULL;
  sb->len = 0;
  sb->cap = 0;
}

void string_buffer_free(String_Buffer *sb) {
  if(sb) free(sb->data);
  string_buffer_clear(sb);	
}

#define TPRINTF_BUFFER_SIZE 4096

const char *tprintf(String_Buffer *sb, const char *format, ...) {
  
  va_list args;
  va_start(args, format);
#ifdef _MSC_VER
  va_list two = args;
#elif __GNUC__
  va_list two;
  va_copy(two, args);
#endif
  size_t len = vsnprintf(NULL, 0, format, args) + 1;
  va_end(args);

  string_buffer_reserve(sb, sb->len + len);
  vsnprintf(sb->data + sb->len, len, format, two);
  sb->len += len;

  return sb->data + sb->len - len;
}

#endif //STRING_IMPLEMENTATION

#endif //STRING_H
