#ifdef SERVE_WASM
//pass
#endif //SERVE_WASM

#ifdef SERVE_SDL

#include <SDL2/SDL.h>
#define DT (1.f/((float) FPS))

int main(int argc, char** argv) {
  (void) argc;
  (void) argv;

  init();

  int quit = 0;

  SDL_Init(SDL_INIT_VIDEO);

  SDL_Event event;

  SDL_Window *wind = SDL_CreateWindow("SDL2 Pixel Drawing",
				      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, 0);
  
  SDL_Renderer * renderer = SDL_CreateRenderer(wind, -1, 0);
  SDL_Texture * texture = SDL_CreateTexture(renderer,
					    SDL_PIXELFORMAT_ABGR8888,
					    SDL_TEXTUREACCESS_STATIC,
					    WIDTH, HEIGHT);

  Uint32 prev = SDL_GetTicks();    
  while (!quit){

    Uint32 curr = SDL_GetTicks();
    float dt = (curr - prev)/1000.0f;
    prev = curr;
    
    while(SDL_PollEvent(&event)) if(event.type==SDL_QUIT) quit = 1;

    uint32_t *pixels = render(dt);
    SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(Uint32));

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(wind);
  SDL_Quit();  

  return 0;  
}

#endif //SERVE_SDL
#ifdef SERVE_VIDEO

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "yuv4mpeg2.h"

//DEFINE DURATION
//DEFINE FPS
//DEFINE OUTPUT_FILE_PATH
#define DT (1.f/((float) FPS))

int main() {
  init();
  
  size_t width = getWidth();
  size_t height = getHeight();

  FILE *f = fopen(OUTPUT_FILE_PATH, "wb");

  writeHeader(f, width, height, FPS);

  for(int i=0;i<DURATION;i++) {
    printf("ITERATION: %d/%d\n", i, DURATION);
    for(int j=0;j<FPS;j++) {
      uint32_t *pixels = render(DT);
      writeFrame(f, pixels, width*height, width);      
    }
  }

  fclose(f);
  printf("Generated %s\n", OUTPUT_FILE_PATH);
}

#endif //SERVE_VIDEO
