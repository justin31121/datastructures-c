#include <stdio.h>

#define HTTP_IMPLEMENTATION
#include "../../repos/datastructures-c/libs/http.h"

#define PLAYER_IMPLEMENTATION
#include "../../repos/datastructures-c/libs/player.h"

#define DOWNLOAD_IMPLEMENTATION
#include "./download.h"

int read(void *opaque, uint8_t *buf, int buf_size) {
  return 0;
}

int64_t seek(void *opaque, int64_t offset, int whence) {
  return 0;
}

int main() {

  Player player;
  if(!player_init(&player, DECODER_FMT_S16, 2, 44100)) {
    panic("player_init");
  }

  //OPEN FILE

  /*
  int sample_rate = 44100;
  if(!player_device_init(&player, sample_rate)) {
    return false;
  }
  if(!decoder_init(&player.decoder,
		   read,
		   seek, NULL,
		   player.fmt,
		   player.channels,
		   player.volume,
		   player.samples)) {
    return false;
  }
  */
  player_play(&player);

  char in[1024];
  for(;;) {
    int n = _read(0, in, 1024);
    if(memcmp("q\n", in, n) == 0) break;
    thread_sleep(10);
  }

  return 0;
}
