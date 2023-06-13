#include <stdio.h>

#define GUI_IMPLEMENTATION
#define GUI_DRAG_N_DROP
#define GUI_CONSOLE
#include "../libs/gui.h"

#define IO_IMPLEMENTATION
#include "../libs/io.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

int main() {

    bool file_uploaded = false;
    
    unsigned char *img_data = NULL;
    int img_width = 0;
    int img_height = 0;

    Gui gui;
    Gui_Canvas canvas = {800, 800, NULL};
    if(!gui_init(&gui, &canvas, "viewer")) {
	return 1;
    }

    float width = 0;
    float height = 0;
    Gui_Event event;
    while(gui.running) {
	while(gui_peek(&gui, &event)) {
	    if(event.type == GUI_EVENT_KEYPRESS) {
		if(event.as.key == 'Q') {
		    gui.running = false;
		}
	    } else if(event.type == GUI_EVENT_FILEDROP) {
		char path[MAX_PATH];
		HDROP h_drop = (HDROP) event.as.value;
		int count = DragQueryFile(h_drop, 0xffffffff, path, MAX_PATH);
		//TODO: handle multiple files somehow
		if(count > 1) {
		    for(int i=0;i<count;i++) {
			DragQueryFile(h_drop, i, path, MAX_PATH);
			printf("%s\n", path);
		    }		    
		} else {
		    DragQueryFile(h_drop, 0, path, MAX_PATH);

		    bool is_file;
		    if(io_exists(path, &is_file) && is_file) {
			
			img_data = stbi_load(path, &img_width, &img_height, 0, 3);
			if(img_data) {
			    if(file_uploaded) {
				stbi_image_free(img_data);
			    }
			    
			    GLuint textures;
			    glGenTextures(1, &textures);
			    glBindTexture(GL_TEXTURE_2D, textures);

			    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
			    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			    glTexImage2D(GL_TEXTURE_2D,
					 0,
					 GL_RGB,
					 img_width,
					 img_height,
					 0,
					 GL_RGB,
					 GL_UNSIGNED_BYTE,
					 img_data);
			    if(!file_uploaded) {
				glEnable(GL_TEXTURE_2D);
			    }
			    file_uploaded = true;
			} else {
			    fprintf(stderr, "ERROR: Can not load file: '%s'\n", path);
			}
			
		    }		    
		}

		DragFinish(h_drop);
	    }
	}

	gui_get_window_sizef(&gui, &width, &height);
	glViewport(0, 0, width, height);
	glClearColor(1, 0, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	if(file_uploaded) {

	    float ratio = (float) img_width / (float) img_height;
	    float x1, x2, y1, y2;

	    float _img_height = width / ratio;
	    if(_img_height < height) {
		x1 = -1;
		x2 = 1;
		y1 = (height - _img_height) / height - 1.f;
		y2 = ((height - _img_height) / 2.f + _img_height) * 2.f / height - 1.f;
	    } else {
		y1 = -1;
		y2 = 1;
		float _img_width = ratio * height;
		x1 = (width - _img_width) / width - 1.f;
		x2 = ((width - _img_width) / 2.f + _img_width) * 2.f / width - 1.f;
	    }

	    glBegin(GL_QUADS);
	    glTexCoord2f(0, 0); glVertex2f(x1, y2);
	    glTexCoord2f(1, 0); glVertex2f(x2, y2);
	    glTexCoord2f(1, 1); glVertex2f(x2, y1);
	    glTexCoord2f(0, 1); glVertex2f(x1, y1);
	    glEnd();			
	}

	gui_swap_buffers(&gui);
    }
    
    return 0;
}
