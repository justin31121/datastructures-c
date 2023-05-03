#include <stdio.h>

#define GUI_IMPLEMENTATION
#define GUI_OPENGL
#include "../libs/gui.h"

#define IMGUI_RENDERER_IMPLEMENTATION
#include "../libs/imgui.h"

#include "../rsc/musik.h"
#include "../rsc/atlas.h"

int main() {

    Gui gui;
    Gui_Event event;
    if(!gui_init(&gui, NULL, "Imgui")) {
	return 1;
    }

    if(!imgui_init(&gui, &event)) {
	return 1;
    }

    int musik = imgui_add_img(musik_data, musik_width, musik_height);
    int atlas = imgui_add_img(atlas_data, atlas_width, atlas_height);
    
    float width, height;
    while(gui.running) {
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
	imgui_button(vec2f(0, 0), vec2f(100, 100), vec4f(1, 1, 1, 1));
	
	imgui_update();
    }
    
    return 0;
}
