#define GUI_IMPLEMENTATION
#include "../libs/gui.h"

#define RENDERER_IMPLEMENTATION
#include "../libs/renderer.h"

#define PEN_IMPLEMENTATION
#include "../libs/pen.h"

#define WIDTH 800
#define HEIGHT 600

//#define OPENGL

#ifdef OPENGL

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

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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

static unsigned int pixels[WIDTH * HEIGHT];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  (void) hPrevInstance;
  (void) lpCmdLine;

  Gui_Canvas canvas = {WIDTH, HEIGHT, pixels};
  
  Gui gui;
  if(!gui_init(&gui, &canvas, hInstance, nCmdShow, "gui_demo")) {
    return -1;
  }

  Vec2f border;
  Gui_Event event;
  while(gui.running) {
    
    while(gui_peek(&gui, &event)) {
    }

    fill(pixels, WIDTH, HEIGHT, 0xff181818);
    gui_render_canvas(&gui);
  }

  gui_free(&gui);

  return 0;
}

#endif //OPENGL
