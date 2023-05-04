#include <stdio.h>

#define GUI_IMPLEMENTATION
#define GUI_OPENGL
#include "../libs/gui.h"

#define IMGUI_RENDERER_IMPLEMENTATION
#include "../libs/imgui.h"

#include "../rsc/musik.h"

#define FOREGROUND vec4f(0.451, 0.6392, 0.8 , 1)
#define WHITE vec4f(0.8667, 0.8667, 0.8667, 1)

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

    float duration = .5f;
    float slider = 0.f;
    
    float width, height;
    while(gui.running) {
	while(imgui_peek()) {
	    if(event.type == GUI_EVENT_KEYPRESS &&
	       event.key == 'Q')  {
		gui.running = false;
	    }
	}
	
	imgui_get_size(&width, &height);
	
	imgui_img_button(musik, vec2f(width / 2 - musik_width/2, height / 2 - musik_height /2), vec2f(musik_width, musik_height));
	imgui_button(vec2f(0, 0), vec2f(100, 100), vec4f(1, 1, 1, 1));
	
	if(imgui_bar(vec2f(100, height - 100), vec2f(600, 8), FOREGROUND, WHITE, &duration, &slider)) {
	    
	}
	
	imgui_update();
    }
    
    return 0;
}
