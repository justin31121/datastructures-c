#include <stdio.h>

#define GUI_IMPLEMENTATION
#include "../libs/gui.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

int main() {

  const char *filename = "./rsc/landscape.jpg";
  int img_w, img_h;
  unsigned char *img_data = stbi_load(filename, &img_w, &img_h, 0, 3);
  if(!img_data) {
    return 1;
  }

  Gui gui;
  if(!gui_init(&gui, NULL, "viewer")) {
    return 1;
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
	       img_w,
	       img_h,
	       0,
	       GL_RGB,
	       GL_UNSIGNED_BYTE,
	       img_data);

  /*
  glTexSubImage2D(GL_TEXTURE_2D,
		  0,
		  0,
		  0,
		  (GLsizei) demuxer_video.buffer_width,
		  (GLsizei) demuxer_video.buffer_height,
		  GL_RGB,
		  GL_UNSIGNED_BYTE,
		  demuxer_video.buffer + ((video_pos % demuxer_video.cap) * demuxer_video.buffer_width * demuxer_video.buffer_height * bytes_per_pixel));
  */

  glEnable(GL_TEXTURE_2D);
  float width = 0;
  float height = 0;
  Gui_Event event;
  while(gui.running) {
    while(gui_peek(&gui, &event)) {
      if(event.type == GUI_EVENT_KEYPRESS) {
	if(event.as.key == 'Q') {
	  gui.running = false;
	}
      } else if(event.type == GUI_EVENT_MOUSERELEASE) {
	
	
      }
    }
    gui_get_window_sizef(&gui, &width, &height);

    glViewport(0, 0, width, height);
    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-1, 1);
    glTexCoord2f(1, 0); glVertex2f(1, 1);
    glTexCoord2f(1, 1); glVertex2f(1, -1);
    glTexCoord2f(0, 1); glVertex2f(-1, -1);
    glEnd();

    gui_swap_buffers(&gui);
  }
  
  return 0;
}
