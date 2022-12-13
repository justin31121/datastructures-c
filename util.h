#ifndef UTIL_H_H
#define UTIL_H_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

void panic(const char* message);

char *slurp_file(const char* file_path, size_t *size);
void write_file(const char *file_path, const char *data);
void write_file_len(const char *file_path, const char *data, size_t size);

int cstr_index_of(const char* cstr, size_t cstr_size, const char *val, size_t val_size);
bool cstr_contains(const char *cstr, size_t cstr_size, const char *val, size_t val_size);

size_t cstr_len(const char *cstr);

//BEGIN string

typedef struct {
  size_t len;
  const char* data;
} string;

#define STRING(cstr_lit) string_from_parts(cstr_lit, sizeof(cstr_lit) - 1)
#define STRING_FMT "%.*s"
#define STRING_ARG(s) (int) (s).len, (s).data

string string_from_parts(const char* data, size_t count);
string string_from_cstr(const char* data);
string string_trim_left(string s);
string string_trim_right(string s);
string string_trim(string s);
string string_chop_by_delim(string *s, char delim);
string string_chop_left(string *s, size_t n);
bool string_chop_int(string *s, int *n);
bool string_chop_int64_t(string *s, int64_t *n);

char* string_to_cstr(string *s);
bool string_to_int(const string s, int *n);

//END string

#ifdef UTIL_IMPLEMENTATION

//BEGIN string

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

string string_from_parts(const char* data, size_t len) {
  string s;
  s.len = len;
  s.data=data;
  return s;
}

string string_from_cstr(const char* data) {
  return string_from_parts(data, strlen(data));
}

string string_trim_left(string s) {
  size_t i = 0;
  while(i<s.len && isspace(s.data[i])) {
    i++;
  }
  return string_from_parts(s.data+i, s.len-i);
}

string string_trim_right(string s) {
  size_t i = 0;
  while(i<s.len && isspace(s.data[s.len - 1 - i])) {
    i++;
  }
  return string_from_parts(s.data, s.len-i);
}

string string_trim(string s) {
  return string_trim_right(string_trim_left(s));
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
  
  string result = string_from_parts(s->data, i);
  
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

  string result = string_from_parts(s->data, n);

  s->data += n;
  s->len -= n;

  return result;
}

//END string

void panic(const char* message) {
  fprintf(stderr, "ERORR: %s\n", message);
  exit(1);
}

size_t cstr_len(const char *cstr) {
  size_t size = 0;
  while(cstr[size++] != 0);
  return --size;
}

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

void write_file(const char *file_path, const char *data) {
  write_file_len(file_path, data, cstr_len(data));
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
    fprintf(stderr, "ERORR: Can not open file '%s' because: %s\n", file_path, strerror(errno));
    exit(1);
  }

  if(fseek(f, 0, SEEK_END) < 0) {
    fprintf(stderr, "ERORR: Can not read file '%s' because: %s\n", file_path, strerror(errno));
    fclose(f);
    exit(1);    
  }

  long m = ftell(f);
  if(m < 0) {
    fprintf(stderr, "ERORR: Can not read file '%s' because: %s\n", file_path, strerror(errno));
    fclose(f);
    exit(1);   
  }  

  if(fseek(f, 0, SEEK_SET) < 0) {
    fprintf(stderr, "ERORR: Can not read file '%s' because: %s\n", file_path, strerror(errno));
    fclose(f);
    exit(1);    
  }

  res = (char *) malloc(m + 1);
  if(!res) {
    fprintf(stderr, "ERORR: Can not allocate enough memory for file '%s'\n", file_path);
    fclose(f);
    exit(1);
  }

  size_t _m = (size_t) m;
  size_t n = fread(res, 1, _m, f);
  if(n != _m) {
    fprintf(stderr, "ERORR: Can not read file '%s' because: %s\n", file_path, strerror(errno));
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
