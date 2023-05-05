#define PLAYER_IMPLEMENTATION
#include "../libs/player.h"

#define GUI_OPENGL
#define IMGUI_RENDERER_IMPLEMENTATION
#include "../libs/imgui.h"

#define WIDTH 320
#define HEIGHT 344
#define BUTTON_WIDTH 24
#define BAR_Y 60

#include "../rsc/musik.h"
#include "../rsc/atlas.h"

int main() {

  Player player;
  if(!player_init(&player, DECODER_FMT_S16, 2, 44100)) {
    return 1;
  }

  if(!player_open_file(&player, ".\\rsc\\doctor.m4a")) {
    return 1;
  }

  Gui gui;
  Gui_Canvas canvas = {WIDTH, HEIGHT, NULL};
  Gui_Event event;
  if(!gui_init(&gui, &canvas, "Imgui")) {
    return 1;
  }

  if(!imgui_init(&gui, &event)) {
    return 1;
  }
  imgui_set_background(0xff181818);
  int musik = imgui_add_img(musik_data, musik_width, musik_height);
  int atlas = imgui_add_img(atlas_data, atlas_width, atlas_height);

  if(!player_play(&player)) {
    return 1;    
  } 

  float width, height;
  while(gui.running) {
    while(imgui_peek()) {
      if(event.type == GUI_EVENT_KEYPRESS &&
	 event.key == 'Q')  {
	gui.running = false;
      }
    }
    imgui_get_size(&width, &height);
    
    imgui_img(musik, vec2f(WIDTH/2 - BUTTON_WIDTH/2 - musik_width/2, BAR_Y + 1.5*BUTTON_WIDTH),
	      vec2f(musik_width, musik_height));    
    if(imgui_subimg_button(atlas, vec2f(WIDTH/2 - BUTTON_WIDTH/2, BUTTON_WIDTH/2),
			vec2f(BUTTON_WIDTH, BUTTON_WIDTH),
			   vec2f(player.playing ? .25 : 0, 0), vec2f(.25, 1))) {
      player_toggle(&player);
    }
    if(imgui_subimg_button(atlas, vec2f(WIDTH/2 - BUTTON_WIDTH/2 + 2*BUTTON_WIDTH, BUTTON_WIDTH/2),
			   vec2f(BUTTON_WIDTH, BUTTON_WIDTH),
			   vec2f(.75 0), vec2f(.25, 1))) {
      
    }
    if(imgui_subimg_button(atlas, vec2f(WIDTH/2 - BUTTON_WIDTH/2 - 2*BUTTON_WIDTH, BUTTON_WIDTH/2),
			   vec2f(BUTTON_WIDTH, BUTTON_WIDTH),
			   vec2f(.5, 0), vec2f(.25, 1))) {
      
    }
    
    imgui_update();
  }
  
  return 0;
}
