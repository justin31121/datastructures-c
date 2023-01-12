#ifndef PARSE_H_H
#define PARSE_H_H

bool isDigit(char c);
bool isHexDigit(char c);
bool normalChar(char c);
bool isSpace(char c);

bool parseChar(char c, const char *buffer, size_t buffer_size);
bool parseCharIf(bool (*f)(char c), const char *buffer, size_t buffer_size);
size_t parseString(const char* target, size_t target_size, const char* buffer, size_t buffer_size);
size_t parseStringIf(bool (*f)(char c), const char *buffer, size_t buffer_size);

size_t skipCharIf(bool (*f)(char c), const char *buffer, size_t buffer_size);

#ifdef PARSE_IMPLEMENTATION

bool isDigit(char c) {
  return '0' <= c && c <= '9';
}

bool isHexDigit(char c) {
  return ('0' <= c && c <= '9') || ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f');
}

bool normalChar(char c) {
  return c!='\"' && c!='\\';
}

bool isSpace(char c) {
  return c==' ' || c=='\n' || c=='\t' || c=='\v' || c=='\f' || c=='\r';
}

bool parseChar(char c, const char *buffer, size_t buffer_size) {
  if(buffer_size==0) return false; 
  if(c==buffer[0]) return true;
  else return false;
}

bool parseCharIf(bool (*f)(char c), const char *buffer, size_t buffer_size) {
  if(buffer_size==0) return false;
  if(f(buffer[0])) return true;
  else return false;
}

size_t parseString(const char* target, size_t target_size, const char* buffer, size_t buffer_size) {
  for(size_t i=0;i<target_size;i++) {
    if(parseChar(target[i], buffer+i, buffer_size-i)==0) return 0;
  }

  return target_size;
}

size_t parseStringIf(bool (*f)(char c), const char *buffer, size_t buffer_size) {
  size_t i = 0;
  for(;i<buffer_size;i++) {
    if(parseCharIf(f, buffer+i, (size_t) buffer_size-i)==0) break;
  }

  return i;
}

size_t skipCharIf(bool (*f)(char c), const char *buffer, size_t buffer_size) {
  size_t i=0;
  while(parseCharIf(f, buffer+i, buffer_size-i)!=0) i++;
  return i;
}


#endif

#endif
