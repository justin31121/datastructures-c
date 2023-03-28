#define GUI_IMPLEMENTATION
#include "../libs/gui.h"

//#define OPENGL

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

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../thirdparty/stb_truetype.h"

////////////////////////////////////

#define PEN_IMPLEMENTATION
#include "../libs/pen.h"

#define FONT_IMPLEMENTATION
#include "../libs/font.h"

#include "../libs/util.h"

#define WIDTH 1280
#define HEIGHT 720

static unsigned int pixels[WIDTH * HEIGHT];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  (void) hPrevInstance;
  (void) lpCmdLine;

  Font font;
  if(!font_init(&font, "C:\\Windows\\Fonts\\arial.ttf", 24)) {
    return -1;
  }

  s32 img_w, img_h;
  u32 *img = (u32 *) stbi_load("./test.jpg", &img_w, &img_h, NULL, 4);
  
  fill(pixels, WIDTH, HEIGHT, 0xff181818);
  copy2(pixels, WIDTH, HEIGHT, img, img_w, img_h, WIDTH - img_w / 2, HEIGHT - img_h / 2, img_w / 2, img_h / 2);
  char *word = "The quick brown fox jumps over the lazy dog!?.";
  u8 n = 20;
  for(u8 i=1;i<n+1;i++) {
      font_render(&font, word, strlen(word),
		  pixels, WIDTH, HEIGHT,
		  0, HEIGHT - i*font.height, ((0xff * (i-1) / n) << 24) | 0x00ffffff);
  }

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
  stbi_image_free(img);
  font_free(&font);

  return 0;
}

#endif //OPENGL
