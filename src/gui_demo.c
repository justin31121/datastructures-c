#define GUI_IMPLEMENTATION
#include "../libs/gui.h"

#define RENDERER_IMPLEMENTATION
#include "../libs/renderer.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  (void) hPrevInstance;
  (void) lpCmdLine;
  
  Gui gui;
  if(!gui_init(&gui, hInstance, nCmdShow, "gui_demo")) {
    return -1;
  }

  if(!gui_init_opengl(&gui)) {
    return -1;
  }

  Renderer renderer;
  if(!renderer_init(&renderer)) {
    return -1;
  }

  Gui_Event event;
  Vec2f border;
  while(gui.running) {
    while(gui_peek(&gui, &event)) {
    }

    gui_get_window_sizef(&gui, &border.x, &border.y);
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
