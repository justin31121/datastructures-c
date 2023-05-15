#include <stdio.h>

#define IO_IMPLEMENTATION
#include "../libs/io.h"

int main() {

  const char *file_path = "./rsc/test.txt";
  Io_File file;
  if(!io_file_open(&file, file_path)) {
    return 1;
  }

  char buf[11];

  io_file_fseek(&file, 10, SEEK_CUR);
  io_file_fseek(&file, 5, SEEK_SET);
  
  printf("size: %ld\n", file.size);
  printf("pos: %ld\n", file.pos);
  
  while(!io_file_feof(&file)) {
    size_t n = io_file_fread(buf, 1, 10, &file);
    buf[n] = 0;
    printf("'%s' (%zd) (%ld)\n", buf, n, file.pos);
  }

  io_file_close(&file);
  
  return 0;
}
