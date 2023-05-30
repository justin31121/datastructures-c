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

#ifndef STRING_DEF
#define STRING_DEF static inline
#endif //STRING_DEF

#include "util.h"

typedef struct {
    const char* data;
    size_t len;
} string;

#define STRING_STATIC(cstr_lit) {.len=sizeof(cstr_lit), .data=cstr_lit}
#define STRING(cstr_lit) string_from(cstr_lit, sizeof(cstr_lit) -1)
#define String_Fmt "%.*s"
#define String_Arg(s) ((int) (s).len), (s).data

STRING_DEF string string_from(const char* data, size_t count);
STRING_DEF string string_from_cstr(const char* data);
STRING_DEF string string_trim_left(string s);
STRING_DEF string string_trim_right(string s);
STRING_DEF string string_trim(string s);
STRING_DEF string string_chop_by_delim(string *s, char delim);
STRING_DEF string string_chop_left(string *s, size_t n);
STRING_DEF string string_chop_right(string *s, size_t n);
STRING_DEF bool string_chop_string(string *s, string dst);
STRING_DEF bool string_chop_cstr(string *s, const char *dst);
STRING_DEF bool string_chop_int(string *s, int *n);
STRING_DEF bool string_chop_hex(string *s, uint64_t *n);
STRING_DEF bool string_chop_int64_t(string *s, int64_t *n);

STRING_DEF int string_index_of(string s, const char *cstr);
STRING_DEF int string_index_of_offset(string s, const char *cstr, size_t offset);

STRING_DEF int string_index_of2(string s, string t);
STRING_DEF int string_index_of_offset2(string s, string t, size_t offset);

STRING_DEF string string_substring(string s, size_t start, size_t end);
STRING_DEF bool string_eq(string s, string t);
STRING_DEF bool string_eq_cstr(string s, const char *cstr);

STRING_DEF char* string_to_cstr(string *s);
STRING_DEF string string_copy(string s);
STRING_DEF string string_copy_cstr(const char *cstr);

STRING_DEF bool cstr_contains(const char *cstr, size_t cstr_size, const char *val, size_t val_size);
STRING_DEF int cstr_index_of(const char* cstr, size_t cstr_size, const char *val, size_t val_size);


///////////////////////////////////////

STRING_DEF bool string_replace(string s, const char *from, const char *to, char *buffer, size_t buffer_cap, size_t *buffer_size);

//STRING_BUFFER

typedef struct{
    char *data;
    size_t len;
    size_t cap;
}String_Buffer;

STRING_DEF bool string_buffer_append(String_Buffer *sb, const char *data, size_t data_size);
STRING_DEF bool string_buffer_append_string(String_Buffer *sb, string s);
STRING_DEF bool string_buffer_reserve(String_Buffer *sb, size_t data_size);

STRING_DEF size_t string_buffer_callback(const void *data, size_t size, size_t memb, void *userdata);

STRING_DEF void string_buffer_free(String_Buffer *sb);

typedef bool (*string_buffer_map)(const char *input, size_t input_size, char *buffer, size_t buffer_size, size_t *output_size);

STRING_DEF const char *tprintf(String_Buffer *sb, const char *format, ...);
STRING_DEF string tsprintf(String_Buffer *sb, const char *format, ...);
STRING_DEF string tsmap(String_Buffer *sb, string input, string_buffer_map map);

#ifdef STRING_IMPLEMENTATION

STRING_DEF bool cstr_contains(const char *cstr, size_t cstr_size, const char *val, size_t val_size) {
    return cstr_index_of(cstr, cstr_size, val, val_size) >= 0;
}

STRING_DEF int cstr_index_of(const char* cstr, size_t cstr_size, const char *val, size_t val_size) {
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

STRING_DEF bool string_chop_int64_t(string *s, int64_t *n) {
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

STRING_DEF bool string_chop_int(string *s, int *n) {
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

STRING_DEF bool string_chop_hex(string *s, uint64_t *n) {
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

STRING_DEF string string_from(const char* data, size_t len) {
    string s;
    s.len = len;
    s.data = data;
    return s;
}

STRING_DEF string string_from_cstr(const char* data) {
    return string_from(data, strlen(data));
}

STRING_DEF string string_trim_left(string s) {
    size_t i = 0;
    while(i<s.len && isspace(s.data[i])) {
	i++;
    }
    return string_from(s.data+i, s.len-i);
}

STRING_DEF string string_trim_right(string s) {
    size_t i = 0;
    while(i<s.len && isspace(s.data[s.len - 1 - i])) {
	i++;
    }
    return string_from(s.data, s.len-i);
}

STRING_DEF string string_trim(string s) {
    return string_trim_right(string_trim_left(s));
}

STRING_DEF int string_index_of(string s, const char *cstr) {
    return cstr_index_of(s.data, s.len, cstr, strlen(cstr));
}

STRING_DEF int string_index_of_offset(string s, const char *cstr, size_t offset) {
    int pos = cstr_index_of(s.data + offset, s.len - offset, cstr, strlen(cstr));
    if(pos < 0) return pos;
    else return pos + (int) offset;
}

STRING_DEF int string_index_of2(string s, string t) {
    return cstr_index_of(s.data, s.len, t.data, t.len);
}

STRING_DEF int string_index_of_offset2(string s, string t, size_t offset) {
    int pos = cstr_index_of(s.data + offset, s.len - offset, t.data, t.len);
    if(pos < 0) return pos;
    else return pos + (int) offset;
}

STRING_DEF string string_substring(string s, size_t start, size_t end) {
    if(start >= s.len || end > s.len) {
	return (string) {0};
    }
    return (string) {.data = s.data + start, .len = end - start};
}

STRING_DEF bool string_eq(string s, string t) {
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

STRING_DEF bool string_eq_cstr(string s, const char *cstr) {
    string cstr_string = string_from_cstr(cstr);
    return string_eq(s, cstr_string);
}

STRING_DEF void string_in_cstr(string s, char* target) {
    for(size_t i=0;i<s.len;i++) {
	target[i]=s.data[i];
    }

    target[s.len]='\0';
}

STRING_DEF char* string_to_cstr(string *s) {
    char* res = (char *) malloc( s->len * sizeof(char) );
    if(!res) {
	panic("Not enough memory");
    }
  
    for(size_t i=0;i<s->len;i++) {
	res[i]=s->data[i];
    }
    res[s->len]='\0';
    return res;
}

STRING_DEF string string_copy(string s) {
    char * data = malloc(s.len);
    if(!data) {
	panic("Not enough memory");
    }
    memcpy(data, s.data, s.len);
    return (string) {.data = data, .len = s.len};
}

STRING_DEF string string_copy_cstr(const char *cstr) {
    size_t len = strlen(cstr);
    char * data = malloc(len);
    if(!data) {
	panic("Not enough memory");
    }
    memcpy(data, cstr, len);
    return (string) {.data = data, .len = len};
}

STRING_DEF string string_chop_by_delim(string *s, char delim) {
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

STRING_DEF string string_chop_left(string *s, size_t n) {
    if(n > s->len) {
	n = s->len;
    }

    string result = string_from(s->data, n);

    s->data += n;
    s->len -= n;

    return result;
}

STRING_DEF string string_chop_right(string *s, size_t n) {
    if(n > s->len) {
	n = s->len;
    }

    string result = string_from(s->data - (s->len + n), s->len - n);

    s->len -= n;

    return result;
}

STRING_DEF bool string_chop_string(string *s, string dst) {
    int pos = string_index_of2(*s, dst);
    if(pos != 0) {
	return false;
    }
    string_chop_left(s, dst.len);
    return true;
}

STRING_DEF bool string_chop_cstr(string *s, const char *dst) {
    string dst_string = string_from_cstr(dst);
    int pos = string_index_of2(*s, dst_string);
    if(pos != 0) {
	return false;
    }
    string_chop_left(s, dst_string.len);
    return true;
}

STRING_DEF size_t string_buffer_callback(const void *data, size_t size, size_t memb, void *userdata) {
    return string_buffer_append((String_Buffer *) userdata, data, size*memb) ? size : 0;
}

STRING_DEF bool string_buffer_send_callback(const char *data, size_t data_size, void *_sb) {
    String_Buffer *sb = (String_Buffer *) _sb;
    return string_buffer_append(sb, data, data_size);
}

STRING_DEF bool string_buffer_append(String_Buffer *sb, const char *data, size_t data_size) {
    if(!sb) {
	return false;
    }

    if(data_size == 0) {
	return true;
    }

    size_t new_cap = sb->cap == 0 ? 64 : sb->cap;
    while(sb->len + data_size >= new_cap) new_cap *=2;
    if(new_cap != sb->cap) {
#ifdef STRING_DEBUG
	size_t old_cap = sb->cap;
	sb->cap = new_cap;
	void *before = sb->data;
	sb->data = (char *) realloc(sb->data, sb->cap);    
	fprintf(stderr, "reallocation from %p (%zd) to %p (%zd)\n", before, old_cap, sb->data, new_cap); fflush(stdout);
#else
	sb->cap = new_cap;
	sb->data = (char *) realloc(sb->data, sb->cap);           	
#endif
	if(!sb->data) {
	    return false;
	}
    }
    memcpy(sb->data + sb->len, data, data_size);
    sb->len += data_size;
    return true;
}

STRING_DEF bool string_buffer_reserve(String_Buffer *sb, size_t data_size) {
    if(!sb) {
	return false;
    }

    if(data_size == 0) {
	return true;
    }

    if(sb->cap >= data_size) {
	return true;
    }

    size_t new_cap = sb->cap == 0 ? 512 : sb->cap;
    while(data_size > new_cap) new_cap*=2;
#ifdef STRING_DEBUG
    size_t old_cap = sb->cap;
    sb->cap = new_cap;
    void *before = sb->data;;
    sb->data = (char *) realloc(sb->data, sb->cap);
    fprintf(stderr, "reallocation from %p (%zd) to %p (%zd)\n", before, old_cap, sb->data, new_cap); fflush(stdout);
#else
    sb->cap = new_cap;
    sb->data = (char *) realloc(sb->data, sb->cap);
#endif
    if(!sb->data) {
	return false;
    }

    return true;
}

STRING_DEF bool string_buffer_append_string(String_Buffer *sb, string s) {
    return string_buffer_append(sb, s.data, s.len);
}

STRING_DEF void string_buffer_clear(String_Buffer *sb) {
    if(!sb) return;
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

STRING_DEF void string_buffer_free(String_Buffer *sb) {
    if(sb) free(sb->data);
    string_buffer_clear(sb);	
}

STRING_DEF const char *tprintf(String_Buffer *sb, const char *format, ...) {
  
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

STRING_DEF string tsprintf(String_Buffer *sb, const char *format, ...) {
    (void) format;
    (void) sb;

    va_list args;
    va_start(args, format);
#ifdef _MSC_VER
    va_list two = args;
#elif __GNUC__
    va_list two;
    va_copy(two, args);
#endif
    size_t len = vsnprintf(NULL, 0, format, args);
    va_end(args);
  
    string_buffer_reserve(sb, sb->len + len);
    vsnprintf(sb->data + sb->len, len, format, two);
    sb->len += len;

    return string_from(sb->data + sb->len - len, len);
}

STRING_DEF bool string_replace(string s, const char *_from, const char *_to, char *buffer, size_t buffer_cap, size_t *buffer_size) {
    string from = string_from_cstr(_from);
    string to = string_from_cstr(_to);

    *buffer_size = 0;

    while(s.len) {
	int pos = string_index_of2(s, from);
	if(pos == -1) {
	    if(buffer_cap < s.len) {
		return false;
	    } 
	    memcpy(buffer + *buffer_size, s.data, s.len);
	    *buffer_size += s.len;
	    buffer_cap -= s.len;
	    s.len = 0; // at this point, just discard s
	} else if(pos == 0) {
	    string_chop_left(&s, from.len);
	    if(buffer_cap < to.len) {
		return false;
	    }
	    memcpy(buffer + *buffer_size, to.data, to.len);
	    *buffer_size += to.len;
	    buffer_cap -= to.len;    
	} else {      
	    size_t _pos = (size_t) pos;
	    if(buffer_cap < _pos) {
		return false;
	    }
	    memcpy(buffer + *buffer_size, s.data, _pos);
	    *buffer_size += _pos;
	    buffer_cap -= _pos;
	    string_chop_left(&s, _pos + from.len);
	    ///////////////////////////
	    if(buffer_cap < to.len) {
		return false;
	    }
	    memcpy(buffer + *buffer_size, to.data, to.len);
	    *buffer_size += to.len;
	    buffer_cap -= to.len;
	}    
    }

    return true;
}

//typedef bool (*string_buffer_map)(const char *input, size_t input_size, char *buffer, size_t buffer_size, size_t *output_size);

//TODO: This is kinda try n error
STRING_DEF string tsmap(String_Buffer *sb, string input, string_buffer_map map) {

    string_buffer_reserve(sb, sb->len + input.len);
    size_t cap = sb->cap;

    size_t output_size;  
    bool could_map = map(input.data, input.len, sb->data + sb->len, sb->cap - sb->len, &output_size);
    while(!could_map) {
	cap *= 2;
	string_buffer_reserve(sb, cap);
	could_map = map(input.data, input.len, sb->data + sb->len, sb->cap - sb->len, &output_size);
    }
    sb->len += output_size;
  
    return (string) {.data = sb->data + sb->len - output_size, .len = output_size};
}

#endif //STRING_IMPLEMENTATION

#endif //STRING_H
