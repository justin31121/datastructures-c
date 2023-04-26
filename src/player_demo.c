#include <stdio.h>

#define STRING_IMPLEMENTATION
#include "../libs/string.h"

#define GUI_IMPLEMENTATION
#define GUI_OPENGL
#include "../libs/gui.h"

#define RENDERER_IMPLEMENTATION
#include "../libs/renderer.h"

#define FONT_IMPLEMENTATION
#include "../libs/font.h"

#define PLAYER_IMPLEMENTATION
#include "../libs/player.h"

#include "../rsc/atlas.h"
#include "../rsc/musik.h"
#include "../rsc/segoeui.h"

#define BACKGROUND 0.09412, 0.09412, 0.09412, 1.0
#define FOREGROUND vec4f(0.451, 0.6392, 0.8 , 1) //0xFFCCA37e
#define WHITE vec4f(0.8667, 0.8667, 0.8667, 1)

#define WIDTH 340
#define HEIGHT 340

void render_line(Renderer *renderer,
		 float x, float y,
		 Font2 *font, const char *word) {
  float word_x = x;
  float word_y = y;

  s32 k = 0;
  while(word[k]) {
    char c = word[k++];
    if(c == ' ') {
      word_x += font->height / 3;
      continue;
    }
	
    float char_width = (float) font->ws[c - 32] / (float) font->width;
    float char_off = (float) (c - 32) * (float) font->height / (float) font->width;

    Vec2f char_size = vec2f(font->ws[c - 32], font->height);
    word_x += font->xs[c -32];

    renderer_text(renderer,
		  vec2f(word_x, word_y - font->ys[c - 32] - font->height),
		  char_size,
		  vec2f(char_off, 0),
		  vec2f(char_width, 1),
		  vec4f(1, 1, 1, 1));
    word_x += char_size.x;
  }

}

String_Buffer sb = {0};

int main() {
    
    Gui gui;
    Gui_Canvas canvas = {WIDTH, HEIGHT, NULL};
    if(!gui_init(&gui, &canvas, "Jlayer")) {
	return -1;
    }
    gui_use_vsync(1);

    Renderer renderer = {0};
    if(!renderer_init(&renderer)) {
	return -1;
    }

    Font2 font = font2_segoeui_22;
    u32 font_tex = renderer_push_texture(&renderer, font.width, font.height, (char *) font.data, true);
    u32 atlas_tex = renderer_push_texture(&renderer, atlas_width, atlas_height, (char *) atlas_data, false);
    u32 musik_tex = renderer_push_texture(&renderer, musik_width, musik_height, (char *) musik_data, false);

    Player player;
    if(!player_init(&player, DECODER_FMT_S16, 2, 44100)) { // for now the best setup
	panic("player_init");
    }
    if(!player_open_file(&player, "./rsc/wraith.m4a")) {
	panic("player_open_file");
    }
    
    if(!player_play(&player)) {
	panic("player_play");
    }
    player_set_volume(&player, 0.2f);

    float button_width = 24.f;
    float bar_height = 60.f;
    bool drag = false;
    bool was_paused = false;

    float cursor_base_x = 0.f;
    float bar_width = 0.f;

    Gui_Time time;
    gui_time_capture(&time);
    u32 acc = 0;
    
    Vec2f border;
    Gui_Event event;
    while(gui.running) {
	bool click = false;
	sb.len = 0;

	u32 ms = (u32) gui_time_measure(&time);
	if(player.playing) acc += ms;
	
	while(gui_peek(&gui, &event)) {
	    if(event.type == GUI_EVENT_KEYPRESS) {
		switch(event.key) {
		case 'Q': {
		    gui.running = false;		    
		} break;
		case 'P': {
		    player_toggle(&player);
		} break;
		}

	    } else if(event.type == GUI_EVENT_MOUSEPRESS) {
		if(event.key == 'L') {
		  click = true;
		}
	    } else if(event.type == GUI_EVENT_MOUSERELEASE) {

	      if(drag) {
		float x = (float) event.mousex - 8.f;
		if(x < cursor_base_x) {
		  x = cursor_base_x;
		}
		if(x + 16 >= cursor_base_x + bar_width) {
		  x = cursor_base_x + bar_width - 16;
		}

		float _time = (x - cursor_base_x) * player.duration_abs / (bar_width - 8.f);
		
		player_seek(&player, _time);
		drag = false;
		if(!was_paused) {
		  player_play(&player); 
		}		
	      }
	    }
	}
	
	gui_get_window_sizef(&gui, &border.x, &border.y);	
	bar_width = border.x * 0.55f;
	cursor_base_x = border.x/2.f - bar_width / 2.f;

	float mousex = (float) event.mousex;
	float mousey = (float) event.mousey;
	gui_mouse_to_screenf(border.x, border.y, &mousex, &mousey);

	renderer.time = (float) acc / 1000.f;
	renderer.resolution = border;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glClearColor(BACKGROUND);
	glClear(GL_COLOR_BUFFER_BIT);

	renderer.image = font_tex;
	renderer_set_shader(&renderer, SHADER_FOR_TEXT);
	
	float _time;
	if(drag) {
	  float x = (float) event.mousex - 8.f;
	  if(x < cursor_base_x) {
	    x = cursor_base_x;
	  }
	  if(x + 16>= cursor_base_x + bar_width) {
	    x = cursor_base_x + bar_width - 16;
	  }
	    
	  float secs = (x - cursor_base_x) * player.duration_abs / (bar_width - 8.f);
	  _time = floorf(secs / 60.f);
	  _time += (float) ((int) secs % 60) / 100;
	} else {
	  player_get_timestamp(&player, &_time);
	}

	const char *text = tprintf(&sb, "%.2f", _time);
	render_line(&renderer,
		    cursor_base_x*.5f - .5f * (float) font_estimate_width2(&font, text), bar_height,
		    &font, text);

	player_get_duration(&player, &_time);
	text = tprintf(&sb, "%.2f", _time);
	render_line(&renderer,
		    border.x - 
		    cursor_base_x*.5f -
		    .5f * (float) font_estimate_width2(&font, text), bar_height,
		    &font, text);
	renderer_flush(&renderer);
	
	
	renderer.image = atlas_tex;
	renderer_set_shader(&renderer, SHADER_FOR_IMAGE);

	//PLAY N PAUSE
	Vec2f pos = vec2f(border.x*.5f - button_width*.5f, 4 + button_width*.5f);
	if(click &&
	   mousex > pos.x &&
	   mousey > pos.y &&
	   mousex - pos.x < button_width &&
	   mousey - pos.y < button_width) {
	  player_toggle(&player);
	}
	renderer_image_rect(&renderer,
			    pos,
			    vec2f(button_width,  button_width),
			    vec2f(player.playing ? .25 : 0, 0), vec2f(.25, 1));

	//PREV
	renderer_image_rect(&renderer, vec2f_sub(pos, vec2f(2*button_width, 0)), vec2f(button_width,  button_width), vec2f(.5, 0), vec2f(.25, 1));

	//NEXT
	renderer_image_rect(&renderer, vec2f_add(pos, vec2f(2*button_width, 0)), vec2f(button_width,  button_width), vec2f(.75, 0), vec2f(.25, 1));
	
	renderer_flush(&renderer);

	renderer.image = musik_tex;
	renderer_set_shader(&renderer, SHADER_FOR_RIPPLE);
	renderer_image_rect(&renderer,
			    vec2f(border.x*.5f - .5f * (float) musik_width, (border.y)*.5f - .5 * (float) musik_height),
			    vec2f(musik_width, musik_height), vec2f(0, 0), vec2f(1, 1));
	renderer_flush(&renderer);
	
	renderer_set_shader(&renderer, SHADER_FOR_COLOR);
	//BAR
	if(click &&
	   mousex >= cursor_base_x &&
	   mousey >= bar_height &&
	   mousex - cursor_base_x < bar_width &&
	   mousey - bar_height < 8) {	  

	  float x = mousex - 8.f;
	  if(x < cursor_base_x) {
	    x = cursor_base_x;
	  }
	  if(x + 16 >= cursor_base_x + bar_width) {
	    x = cursor_base_x + bar_width - 16;
	  }

	  float time_ = (x - cursor_base_x) * player.duration_abs / (bar_width - 8.f);
		
	  player_seek(&player, time_);
	}
	renderer_solid_rect(&renderer,
			    vec2f(cursor_base_x, bar_height),
			    vec2f(bar_width, 8),
			    WHITE);

	//CURSOR
	
	float cursor_pos = cursor_base_x;

	if(player.decoder_used) {
	  float timestamp, duration;
	  player_get_timestamp_abs(&player, &timestamp);
	  player_get_duration_abs(&player, &duration);

	  if(!drag) {
	    cursor_pos += (bar_width - 8.f) * timestamp / duration;
	    
	    if(click &&
	       mousex > cursor_pos &&
	       mousey > (bar_height - 4) &&
	       mousex - cursor_pos < 16 &&
	       mousey - (bar_height - 4) < 16) {
	      drag = true;
	      was_paused = !player_stop(&player);
	    }
	  } else {
	    cursor_pos = mousex - 12;
	    if(cursor_pos < cursor_base_x) {
	      cursor_pos = cursor_base_x;
	    }
	    if(cursor_pos + 16 >= cursor_base_x + bar_width) {
	      cursor_pos = cursor_base_x + bar_width - 16;
	    }
	  } 
	}

	renderer_solid_rect(&renderer, vec2f(cursor_pos, bar_height - 4), vec2f(16, 16), FOREGROUND);
	renderer_flush(&renderer);

	gui_swap_buffers(&gui);
    }

    //gui_free(&gui);

    return 0;
}

