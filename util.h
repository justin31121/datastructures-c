#ifndef UTIL_H_H
#define UTIL_H_H

void panic(const char* message);

char *slurp_file(const char* file_path, size_t *size);
void write_file(const char *file_path, const char *data);
void write_file_len(const char *file_path, const char *data, size_t size);

int cstr_index_of(const char* cstr, size_t cstr_size, const char *val, size_t val_size);
bool cstr_contains(const char *cstr, size_t cstr_size, const char *val, size_t val_size);

size_t cstr_len(const char *cstr);

#ifdef UTIL_IMPLEMENTATION

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
