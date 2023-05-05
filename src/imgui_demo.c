#define GUI_OPENGL
#define IMGUI_RENDERER_IMPLEMENTATION
#include "../libs/imgui.h"

#define STRING_IMPLEMENTATION
#include "../libs/string.h"

#include "../rsc/musik.h"
#include "../rsc/atlas.h"
#include "../rsc/segoeui.h"

#define FOREGROUND vec4f(0.451, 0.6392, 0.8 , 1)
#define WHITE vec4f(0.8667, 0.8667, 0.8667, 1)

String_Buffer temp = {0};

int main() {

  Gui gui;
  Gui_Event event;
  if(!gui_init(&gui, NULL, "Imgui")) {
    return 1;
  }

  if(!imgui_init(&gui, &event)) {
    return 1;
  }

  imgui_set_font(&font2_segoeui_24);
  int musik = imgui_add_img(musik_data, musik_width, musik_height);
  int atlas = imgui_add_img(atlas_data, atlas_width, atlas_height);

  float duration = .5f;
  float slider = 0.f;
  bool playing = true;
    
  float width, height;
  while(gui.running) {
    temp.len = 0;
      
    while(imgui_peek()) {
      if(event.type == GUI_EVENT_KEYPRESS &&
	 event.key == 'Q')  {
	gui.running = false;
      }
    }
	
    imgui_get_size(&width, &height);

    imgui_img(musik,
	      vec2f(width / 2 - musik_width/2, height / 2 - musik_height /2),
	      vec2f(musik_width, musik_height));
    if(imgui_subimg_button(atlas,
			   vec2f(width / 2 - atlas_width / 8, height/2 - atlas_height/2 - musik_height),
			   vec2f(atlas_width / 4, atlas_height),
			   vec2f(playing ? 0 : .25, 0), vec2f(.25, 1))) {
      playing = !playing;
    }
    if(imgui_bar(vec2f(100, height - 100), vec2f(600, 8),
		 FOREGROUND, WHITE, duration, &slider)) {
      duration = slider;
    }
    imgui_text(vec2f(100, 200), tprintf(&temp, "%.2f", slider));
    imgui_text(vec2f(700, 200), tprintf(&temp, "%.2f", 1.0));
	
    imgui_update();
  }
    
  return 0;
}
