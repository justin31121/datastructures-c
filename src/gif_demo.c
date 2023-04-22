#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

#define JSON_IMPLEMENTATION
#include "../libs/json.h"

#define HTTP_IMPLEMENTATION
#include "../libs/http.h"

#define PEN_IMPLEMENTATION
#include "../libs/pen.h"

#define GUI_IMPLEMENTATION
#include "../libs/gui.h"

#define WIDTH 800
#define HEIGHT 600

String_Buffer buffer = {0};

void get(const char* url) {

  buffer.len = 0;
  
  if(!http_get(url, string_buffer_callback, &buffer, NULL, NULL)) {
    panic("http_get");
  }  
}

Json get_emotes() {
  get("https://api.betterttv.net/3/emotes/shared/top");

  Json json;
  bool p = json_parse_len(buffer.data, buffer.len, &json);
  
  if(!p) {
    fprintf(stderr, "json_parse failed()\n");
    exit(1);
  }

  return json;
}

#define Z_CAP 256

static uint32_t src[WIDTH*HEIGHT] = {0};
static uint32_t img[Z_CAP][WIDTH*HEIGHT] = {0};
static float gif_delays[Z_CAP];

static int img_width;
static int img_height;
static int img_z;

static int isPng = -1;

float time_passed = 0.0f;
int z = 0;

u32 *render(float dt) {
  (void) dt;
  fill(src, WIDTH, HEIGHT, 0xff000000);
  size_t width = 200;
  size_t height = width * img_height / img_width;
  if(isPng == -1) {
    circle(src, WIDTH, HEIGHT,
	   WIDTH/2, HEIGHT/2, WIDTH/4, 0xff33ddff);
  }
  else if(isPng == 1) {
    copy(src, WIDTH, HEIGHT,
	 img[0], img_width, img_height,
	 WIDTH/2 - width/2, HEIGHT/2 - height/2,
	 width, height);
  }
  else if(isPng == 0) {
    time_passed += dt;
    if(time_passed>gif_delays[z]) {
      time_passed = 0;
      z = (z+1) % img_z;
    }

    copy(src, WIDTH, HEIGHT,
	 img[z], img_width, img_height,
	 WIDTH/2 - width/2,
	 HEIGHT/2 - height/2,
	 width, height);
  }
  return src;
}

void load_media(Json arr, int index) {

  Json emote = json_get_object(json_opt_object(arr, index), "emote");

  char template[60] = "https://cdn.betterttv.net/emote/xxxxxxxxxxxxxxxxxxxxxxxx/2x";
  template[59] = 0;
  char* id = json_get_string(emote, "id");
  char *ext = json_get_string(emote, "imageType");

  /*
  char buf[ext.size+1];
  memcpy(buf, ext.data, ext.size);
  buf[ext.size] = 0;

  assert(id.size==24);
  */
  memcpy(template + 32, id, 24);

  bool png = strcmp(ext, "gif")!=0;
  
  isPng = -1;
  if(png) {
    get(template);
    int n;
    u32* data =
      (u32 *) stbi_load_from_memory((unsigned char *) buffer.data, buffer.len, &img_width, &img_height, &n, 4);

    memcpy(img[0], data, img_width*img_height*4);
    stbi_image_free(data);
    isPng = 1;
  }  
  else {
    get(template);
    int comp;
    int *delays = NULL;
    char *data = (char *) stbi_load_gif_from_memory((unsigned char *) buffer.data,
						    (int) buffer.len,
						    &delays,
						    &img_width,
						    &img_height,
						    &img_z,
						    &comp, 0);
    if(!data) {
      fprintf(stderr, "ERROR: Data is not read properly\n");
      exit(1);
    }
    if(img_width>WIDTH || img_height>HEIGHT) {
      fprintf(stderr, "ERROR: Read file is too big\n");
      exit(1);
    }
    if(img_z>Z_CAP) {
      fprintf(stderr, "ERROR: Gif has to many images; %d\n", img_z);
      exit(1);    
    }

    int stride = img_width*comp*img_height;
    for(int i=0;i<img_z;i++) {
      memcpy(img[i], data + stride*i, stride);
      gif_delays[i] = delays[i]<=10 ? 0.1f : (((float) delays[i])/1000);
    }

    if(delays) stbi_image_free(delays);
    if(data) stbi_image_free(data);
    z = 0;
    time_passed = 0.0f;
    
    isPng = 0;
  }
}

int main() {

  //INIT
  Json arr = get_emotes();
  int cur = 0;
  int arr_size = json_size(arr);
  load_media(arr, cur);

  //BEGIN SDL
  Gui gui;
  Gui_Canvas canvas = {WIDTH, HEIGHT, src};
  if(!gui_init(&gui, &canvas, "gif")) {
    panic("gui_init");
  }

  Gui_Event event;
  while(gui.running) {
    while(gui_peek(&gui, &event)) {}

    gui_render_canvas(&gui);
  }

  /*
  Uint32 prev = SDL_GetTicks();
  //END SDL

  //MAIN LOOOP
  while (!quit){

    Uint32 curr = SDL_GetTicks();
    float dt = (curr - prev)/1000.0f;
    prev = curr;
    
    while(SDL_PollEvent(&event)) {
      if(event.type==SDL_QUIT) quit = 1;
      else if(event.type==SDL_KEYDOWN) {
	switch(event.key.keysym.sym) {
	case 'a':
	  cur = cur>0 ? cur-1 : cur;
	  load_media(arr, cur);
	  break;
	case 'd':
	  cur = cur<arr_size-1 ? cur+1 : cur;
	  load_media(arr, cur);
	  break;
	}
      }
    }

    uint32_t *pixels = render(dt);
    SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(Uint32));

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }
  */


  return 0;
}
