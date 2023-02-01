#define WATCHER_IMPLEMENTATION
#include "../libs/watcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void panic(const char *message) {
  fprintf(stderr, "ERORR: %s\n", message);
  exit(1);
}

void handle_event(Watcher_Event event, const char* file_name) {
  printf("%s got %s\n", file_name, watcher_event_name(event));
}

int main() {
  Watcher watcher;
  if(!watcher_init(&watcher, "./", handle_event)) {
    panic("watcher_init");
  }

  if(!watcher_start(&watcher)) {
    panic("watcher_start");
  }

  char in[64];
  while(true) {
    scanf("%s", in);
    if(strcmp(in, "q") == 0) break;
  }

  watcher_free(&watcher);
  return 0;
}
