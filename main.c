#include <stdio.h>

#define PLAYER_IMPLEMENTATION
#include "./libs/player.h"

bool callback(const char *data, size_t data_size, void *userdata) {
  fwrite(data, data_size, 1, stdout);
  return true;
}

int main() {

  char buf[4];
  sendf(callback, NULL, buf, sizeof(buf), "Hello, World!\n");
  
  return 0;
}
