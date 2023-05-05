#ifndef IMGUI_H_H
#define IMGUI_H_H

#include <stdbool.h>

#ifdef IMGUI_RENDERER_IMPLEMENTATION

#define FONT_IMPLEMENTATION
#define RENDERER_IMPLEMENTATION
#define GUI_IMPLEMENTATION
#define VEC_IMPLEMENTATION

#endif //IMGUI_RENDERER_IMPLEMENTATION

#include "./font.h"
#include "./gui.h"
#include "./vec.h"

#ifndef IMGUI_DEF
#define IMGUI_DEF static inline
#endif //IMGUI_DEF

typedef struct Imgui Imgui;

IMGUI_DEF bool imgui_init(Gui *gui, Gui_Event *event);

IMGUI_DEF void imgui_rect(Vec2f pos, Vec2f size, Vec4f color);
IMGUI_DEF bool imgui_button(Vec2f pos, Vec2f size, Vec4f color);
IMGUI_DEF bool imgui_bar(Vec2f pos, Vec2f size, Vec4f color, Vec4f background, float value, float *cursor);

IMGUI_DEF void imgui_img(int img, Vec2f pos, Vec2f size);
IMGUI_DEF bool imgui_img_button(int img, Vec2f pos, Vec2f size);

IMGUI_DEF void imgui_subimg(int img, Vec2f pos, Vec2f size, Vec2f uvs, Vec2f uvp);
IMGUI_DEF bool imgui_subimg_button(int img, Vec2f pos, Vec2f size, Vec2f uvs, Vec2f uvp);

IMGUI_DEF void imgui_text(Vec2f pos, const char *text);
IMGUI_DEF void imgui_text_len(Vec2f pos, const char *text, size_t text_len);

IMGUI_DEF int imgui_add_img(unsigned int *img_data, int img_width, int img_height);
IMGUI_DEF void imgui_set_font(Font2 *font);
IMGUI_DEF void imgui_set_background(unsigned int background);

IMGUI_DEF void imgui_get_size(float *width, float *height);
IMGUI_DEF bool imgui_peek();
IMGUI_DEF void imgui_update();

IMGUI_DEF void imgui_free();

#ifdef IMGUI_RENDERER_IMPLEMENTATION

#include "./renderer.h"

struct Imgui{
  Gui *gui;
  Gui_Event *event;
  Vec2f click_input;
  Vec2f input;
  Vec2f pos;
  Vec2f size;
  Renderer_Shader current_shader;
  Font2 *font;
  int font_index;
  Renderer renderer;
  bool clicked;
  bool released;
  float background_r;
  float background_g;
  float background_b;
  float background_a;
};

static bool imgui_running = false;
static Imgui imgui_instance = {0};

#define IMGUI_RENDERER_UPDATE_SHADER(shader) do{			\
    if(imgui_instance.current_shader == (Renderer_Shader) -1) {		\
      imgui_instance.current_shader = (shader);				\
      renderer_set_shader(&imgui_instance.renderer, imgui_instance.current_shader); \
    } else if(imgui_instance.current_shader != (shader)) {		\
      renderer_flush(&imgui_instance.renderer);				\
      imgui_instance.current_shader = (shader);				\
      renderer_set_shader(&imgui_instance.renderer, imgui_instance.current_shader); \
    }									\
  }while(0)

#define IMGUI_RENDERER_UPDATE(img, shader) do{				\
    if(imgui_instance.current_shader == (Renderer_Shader) -1) {		\
      imgui_instance.current_shader = (shader);				\
      imgui_instance.renderer.image = (img);				\
      renderer_set_shader(&imgui_instance.renderer, imgui_instance.current_shader); \
    } else if(imgui_instance.current_shader != (shader)) {		\
      renderer_flush(&imgui_instance.renderer);				\
      imgui_instance.current_shader = (shader);				\
      imgui_instance.renderer.image = (img);				\
      renderer_set_shader(&imgui_instance.renderer, imgui_instance.current_shader); \
    } else if((int) imgui_instance.renderer.image != img) {		\
      renderer_flush(&imgui_instance.renderer);				\
      imgui_instance.renderer.image = (img);				\
      renderer_set_shader(&imgui_instance.renderer, imgui_instance.current_shader); \
    }									\
  }while(0)

#define IMGUI_RENDERER_VEC_IN_RECT(input, pos, size)	\
  (((pos).x <= (input).x) &&				\
   ((input).x < (pos).x + (size).x) &&			\
   ((pos).y <= (input).y) &&				\
   ((input).y < (pos).y + (size).y))

IMGUI_DEF bool imgui_init(Gui *gui, Gui_Event *event) {
  if(imgui_running) return true;

  imgui_instance.gui = gui;
  imgui_instance.event = event;

  imgui_instance.renderer = (Renderer) {0};
  if(!renderer_init(&imgui_instance.renderer)) {
    return false;
  }
  imgui_instance.current_shader = -1;
  imgui_instance.clicked = false;
  imgui_instance.released = false;
  imgui_instance.input = vec2f(-1.f, -1.f);
  imgui_instance.pos = vec2f(-1.f, -1.f);
  imgui_instance.size = vec2f(0.f, 0.f);
  imgui_instance.font = NULL;
  imgui_instance.font_index = -1;
  imgui_instance.background_r = 0;
  imgui_instance.background_g = 0;
  imgui_instance.background_b = 0;
  imgui_instance.background_a = 0;

  imgui_running = true;
  return true;
}

IMGUI_DEF bool imgui_peek() {
  bool ret = gui_peek(imgui_instance.gui, imgui_instance.event);
    
  imgui_instance.pos = vec2f((float) imgui_instance.event->mousex,
			     (float) imgui_instance.event->mousey);
  gui_mouse_to_screenf(imgui_instance.size.x, imgui_instance.size.y,
		       &imgui_instance.pos.x,
		       &imgui_instance.pos.y);
    
  if(imgui_instance.event->type == GUI_EVENT_MOUSEPRESS &&
     imgui_instance.event->key == 'L') {
    imgui_instance.input = imgui_instance.pos;
    imgui_instance.clicked = true;
  } else if(imgui_instance.event->type == GUI_EVENT_MOUSERELEASE) {
    imgui_instance.released = true;
  }
  return ret;
}

IMGUI_DEF void imgui_get_size(float *width, float *height) {    
  gui_get_window_sizef(imgui_instance.gui, width, height);

  imgui_instance.size = vec2f(*width, *height);
  imgui_instance.renderer.resolution = imgui_instance.size;
    
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glClearColor(imgui_instance.background_r,
	       imgui_instance.background_g,
	       imgui_instance.background_b,
	       imgui_instance.background_a);
  glClear(GL_COLOR_BUFFER_BIT);
}

IMGUI_DEF void imgui_rect(Vec2f pos, Vec2f size, Vec4f color) {
  IMGUI_RENDERER_UPDATE_SHADER(SHADER_FOR_COLOR);
  renderer_solid_rect(&imgui_instance.renderer, pos, size, color);
}

IMGUI_DEF bool imgui_bar(Vec2f pos, Vec2f size, Vec4f color, Vec4f background, float value, float *cursor) {
  Vec2f cursor_pos = vec2f_add(pos, vec2f(value * size.x - size.y, -size.y/2));
  Vec2f cursor_size = vec2fs(size.y*2);
  *cursor = value;
    
  bool cursor_clicked = IMGUI_RENDERER_VEC_IN_RECT(imgui_instance.input, cursor_pos, cursor_size);
  if(cursor_clicked) {
    cursor_pos.x = imgui_instance.pos.x - size.y;
    if(cursor_pos.x >= pos.x + size.x) cursor_pos.x = pos.x + size.x - size.y;
    if(cursor_pos.x < pos.x) cursor_pos.x = pos.x - size.y;
    *cursor = (cursor_pos.x - pos.x + size.y) / size.x;
  }
    
  IMGUI_RENDERER_UPDATE_SHADER(SHADER_FOR_COLOR);
  renderer_solid_rect(&imgui_instance.renderer, pos, size, background);
  renderer_solid_rect(&imgui_instance.renderer, cursor_pos, cursor_size, color);

  bool clicked = IMGUI_RENDERER_VEC_IN_RECT(imgui_instance.input, pos, size);
  if(imgui_instance.released && cursor_clicked) {    
    return true;
  }
  if(imgui_instance.released && clicked) {
    *cursor = (imgui_instance.input.x - pos.x) / size.x;
    return true;
  }
  return false;
}

IMGUI_DEF bool imgui_button(Vec2f pos, Vec2f size, Vec4f color) {
  IMGUI_RENDERER_UPDATE_SHADER(SHADER_FOR_COLOR);
  bool holding = IMGUI_RENDERER_VEC_IN_RECT(imgui_instance.input, pos, size);
  if(holding) {
    color.w *= 0.5;
  }
  renderer_solid_rect(&imgui_instance.renderer, pos, size, color);
  return imgui_instance.released && holding;
}

IMGUI_DEF void imgui_img(int img, Vec2f pos, Vec2f size) {
  IMGUI_RENDERER_UPDATE(img, SHADER_FOR_IMAGE);
  renderer_image_rect(&imgui_instance.renderer, pos, size, vec2f(0, 0), vec2f(1, 1));
}

IMGUI_DEF bool imgui_img_button(int img, Vec2f pos, Vec2f size) {
  IMGUI_RENDERER_UPDATE(img, SHADER_FOR_IMAGE);
  bool holding = IMGUI_RENDERER_VEC_IN_RECT(imgui_instance.input, pos, size);
  float alpha = 1.0f;
  if(holding) {
    alpha = .5f;
  }
  renderer_image_rect_alpha(&imgui_instance.renderer, pos, size, vec2f(0, 0), vec2f(1, 1), alpha);
  return imgui_instance.released && holding;    
}

IMGUI_DEF void imgui_subimg(int img, Vec2f pos, Vec2f size, Vec2f uvs, Vec2f uvp) {
  IMGUI_RENDERER_UPDATE(img, SHADER_FOR_IMAGE);
  renderer_image_rect(&imgui_instance.renderer, pos, size, uvs, uvp);
}

IMGUI_DEF bool imgui_subimg_button(int img, Vec2f pos, Vec2f size, Vec2f uvs, Vec2f uvp) {
  IMGUI_RENDERER_UPDATE(img, SHADER_FOR_IMAGE);
  bool holding = IMGUI_RENDERER_VEC_IN_RECT(imgui_instance.input, pos, size);
  float alpha = 1.0f;
  if(holding) {
    alpha = .5f;
  }
  renderer_image_rect_alpha(&imgui_instance.renderer, pos, size, uvs, uvp, alpha);
  return imgui_instance.released && holding;    
}

IMGUI_DEF void imgui_text(Vec2f pos, const char *text) {
  imgui_text_len(pos, text, strlen(text));
}

IMGUI_DEF void imgui_text_len(Vec2f pos, const char *text, size_t text_len) {
  IMGUI_RENDERER_UPDATE(imgui_instance.font_index, SHADER_FOR_TEXT);
  for(size_t k=0;k<text_len;k++) {
    char c = text[k];
    if(c == ' ') {
      pos.x += imgui_instance.font->height / 3;
      continue;
    }
	
    float char_width = (float) imgui_instance.font->ws[c - 32] / (float) imgui_instance.font->width;
    float char_off = (float) (c - 32) * (float) imgui_instance.font->height / (float) imgui_instance.font->width;

    Vec2f char_size = vec2f(imgui_instance.font->ws[c - 32], imgui_instance.font->height);
    pos.x += imgui_instance.font->xs[c -32];

    renderer_text(&imgui_instance.renderer,
		  vec2f(pos.x, pos.y - imgui_instance.font->ys[c - 32] - imgui_instance.font->height),
		  char_size,
		  vec2f(char_off, 0),
		  vec2f(char_width, 1),
		  vec4f(1, 1, 1, 1));
    pos.x += char_size.x;

  }
}

IMGUI_DEF int imgui_add_img(unsigned int *img_data, int img_width, int img_height) {        
  return renderer_push_texture(&imgui_instance.renderer,
			       img_width, img_height,
			       (char *) img_data, false);
}

IMGUI_DEF void imgui_set_font(Font2 *font) {
  imgui_instance.font = font;
  imgui_instance.font_index = renderer_push_texture(&imgui_instance.renderer, font->width, font->height, (char *) font->data, true);
}

IMGUI_DEF void imgui_set_background(unsigned int background) {
  imgui_instance.background_r = (float) (background & 0xff) / 255.f;
  imgui_instance.background_g = (float) ((background & 0xff00) >> 8) / 255.f;
  imgui_instance.background_b = (float) ((background & 0xff0000) >> 16) / 255.f;
  imgui_instance.background_a = (float) ((background & 0xff000000) >> 24) / 255.f;
}

IMGUI_DEF void imgui_update() {
  renderer_flush(&imgui_instance.renderer);
  imgui_instance.current_shader = -1;
  gui_swap_buffers(imgui_instance.gui);

  if(imgui_instance.released) {
    imgui_instance.input = vec2f(-1.f, -1.f);
  }
    
  imgui_instance.clicked = false;
  imgui_instance.released = false;    
}

IMGUI_DEF void imgui_free() {
  if(!imgui_running) return;
  imgui_running = false;
}

#endif //IMGUI_RENDERER_IMPLEMENTATION

#endif //IMGUI_H_H
