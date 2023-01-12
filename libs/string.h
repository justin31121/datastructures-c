#ifndef STRING_H
#define STRING_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

typedef struct {
  size_t len;
  const char* data;
} string;

#define STRING_STATIC(cstr_lit) {.len=sizeof(cstr_lit), .data=cstr_lit}
#define STRING(cstr_lit) string_from(cstr_lit, sizeof(cstr_lit) -1)
#define String_Fmt "%.*s"
#ifdef _WIN32
#define String_Arg(s) ((int) (s).len), (s).data
#endif //_WIN32
#ifdef linux
#define String_Arg(s) ((int) (s).len), (s).data
#endif //linux

string string_from(const char* data, size_t count);
string string_from_cstr(const char* data);
string string_trim_left(string s);
string string_trim_right(string s);
string string_trim(string s);
string string_chop_by_delim(string *s, char delim);
string string_chop_left(string *s, size_t n);
bool string_chop_int(string *s, int *n);
bool string_chop_int64_t(string *s, int64_t *n);

int string_index_of(string s, const char *cstr);
int string_index_of_offset(string s, const char *cstr, size_t offset);
string string_substring(string s, size_t start, size_t end);
bool string_eq(string s, string t);

char* string_to_cstr(string *s);
bool string_to_int(const string s, int *n);

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

#ifdef STRING_IMPLEMENTATION

bool cstr_contains(const char *cstr, size_t cstr_size, const char *val, size_t val_size) {
  return cstr_index_of(cstr, cstr_size, val, val_size) >= 0;
}

int cstr_index_of(const char* cstr, size_t cstr_size, const char *val, size_t val_size) {
  for(size_t i=0;i<cstr_size - val_size;i++) {
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

string string_from(const char* data, size_t len) {
  string s;
  s.len = len;
  s.data=data;
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

bool string_to_int(string s, int *target) {
  char s1[s.len];
  string_in_cstr(s, s1);
  long lnum;
  int num;
  char* end;

  errno = 0;
  lnum = strtol(s1, &end, 10);
  if(end==s1) {
    //fprintf(stderr, "ERROR: Can't convert string to number\n");
    return false;
  }

  if((lnum == LONG_MAX || lnum == LONG_MIN) && errno == ERANGE) {
    // fprintf(stderr, "ERROR: number out of range for LONG\n");
    return false;
  }
  
  if((lnum > INT_MAX) || (lnum < INT_MIN) ) {
    //fprintf(stderr, "ERROR: number out of range for INT\n");
    return false;
  }
  
  num = (int) lnum;
  if(target) (*target) = num;
  return true;
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

size_t string_buffer_callback(const void *data, size_t size, size_t memb, void *userdata) {
  return string_buffer_append((String_Buffer *) userdata, data, size*memb) ? size : 0;
}

bool string_buffer_append(String_Buffer *sb, const char *data, size_t data_size) {
  if(!sb) {
    return false;
  }

  if(data_size == 0) {
    return true;
  }

  if(!data) {
    return false;
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

#endif //STRING_IMPLEMENTATION

#endif //STRING_H
