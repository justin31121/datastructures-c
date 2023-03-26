#define GUI_IMPLEMENTATION
#include "../libs/gui.h"

#define OPENGL

#ifdef OPENGL

#define RENDERER_IMPLEMENTATION
#include "../libs/renderer.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  Gui gui;
  if(!gui_init(&gui, NULL, hInstance, nCmdShow, "gui_demo")) {
    return -1;
  }

  Renderer renderer;
  if(!renderer_init(&renderer)) {
    return -1;
  }

  Vec2f border;
  gui_get_window_sizef(&gui, &border.x, &border.y);
  Gui_Event event;
  while(gui.running) {
    
    gui_get_window_sizef(&gui, &border.x, &border.y);
	
    while(gui_peek(&gui, &event)) {
    }

    renderer.resolution = border;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glClearColor(0.09412f, 0.09412f, 0.09412f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    renderer_set_shader(&renderer, SHADER_FOR_COLOR);
    renderer_solid_rect(&renderer, vec2f(0, 0), vec2f(100, 100), vec4f(0, 1, 0, 1));
    renderer_flush(&renderer);

    gui_swap_buffers(&gui);
  }
  
  gui_free(&gui);
  
  return 0;
}

#else

#define WIDTH 800
#define HEIGHT 600

#define PEN_IMPLEMENTATION
#include "../libs/pen.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../thirdparty/stb_truetype.h"

#define FONT_IMPLEMENTATION
#include "../libs/font.h"

static unsigned int pixels[WIDTH * HEIGHT];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  (void) hPrevInstance;
  (void) lpCmdLine;

  Font font;
  if(!font_init(&font, "C:\\Windows\\Fonts\\arial.ttf", 128)) {
    return -1;
  }
  
  fill(pixels, WIDTH, HEIGHT, 0xff181818);

  char c = 'A';
  for(u32 j=0;j<font.height;j++) {
    for(u32 i=0;i<font.height;i++) {
      char d = font.data[(j * font.width) + ((c - 32) * font.height) + i];
      pixels[(font.height - j - 1)*WIDTH+i] =
	rgba_mix(((d << 24) | (d << 16) | (d << 8) | d), pixels[(font.height - j - 1)*WIDTH+i]);
    }
  }
  rect(pixels, WIDTH, HEIGHT, 0, 0, 200, 100, 0xaa33ddff);  

  Gui gui;
  Gui_Canvas canvas = {WIDTH, HEIGHT, pixels};
  if(!gui_init(&gui, &canvas, hInstance, nCmdShow, "gui_demo")) {
    return -1;
  }

  Gui_Event event;
  while(gui.running) {
    
    while(gui_peek(&gui, &event)) {
    }
    
    gui_render_canvas(&gui);
  }

  gui_free(&gui);
  font_free(&font);

  return 0;
}

#endif //OPENGL
