#include <stdio.h>

#define GUI_IMPLEMENTATION
#define GUI_OPENGL
#include "../libs/gui.h"

#define RENDERER_IMPLEMENTATION
#include "../libs/renderer.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../thirdparty/stb_truetype.h"
#define FONT_IMPLEMENTATION
#include "../libs/font.h"

#include "../libs/util.h"

#define PLAYER_IMPLEMENTATION
#include "../libs/player.h"

#define BACKGROUND 0.09412, 0.09412, 0.09412, 1.0
#define FOREGROUND vec4f(0.451, 0.6392, 0.8 , 1) //0xFFCCA37e
#define WHITE vec4f(0.8667, 0.8667, 0.8667, 1)

#define HEIGHT 600
#define WIDTH 800

void render_line(Renderer *renderer,
		 float x, float y,
		 Font *font, const char *word) {
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
		  vec2f(word_x, word_y - font->ys[c - 32]),
		  char_size,
		  vec2f(char_off, 0),
		  vec2f(char_width, 1),
		  vec4f(1, 1, 1, 1));
    word_x += char_size.x;
  }

}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    Gui gui;
    Gui_Canvas canvas = {WIDTH, HEIGHT, NULL};
    if(!gui_init(&gui, &canvas, hInstance, nCmdShow, "gui_demo")) {
	return -1;
    }
    gui_use_vsync(1);

    Renderer renderer = {0};
    if(!renderer_init(&renderer)) {
	return -1;
    }

    Font font;
    if(!font_init(&font, "C:\\Windows\\Fonts\\arial.ttf", 48)) {
	return -1;
    }

    Renderer_Texture font_texture = {font.width, font.height, (char *) font.data, true};
    u32 font_tex = renderer_push_texture(&renderer, font_texture);

    Player player;
    if(!player_init(&player, DECODER_FMT_S16, 2, 44100)) {
	panic("player_init");
    }
    if(!player_open_file(&player, ".\\rsc\\doctor.m4a")) {
	panic("player_open_file");
    }
    if(!player_play(&player)) {
	panic("player_play");
    }
    player_set_volume(&player, 0.05f);

    float button_width = 32.f;
    
    Vec2f border;
    Gui_Event event;
    while(gui.running) {
	bool click = false;
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
	    }
	}

	gui_get_window_sizef(&gui, &border.x, &border.y);    
	renderer.resolution = border;

	float bar_width = border.x * 0.75f;
	float cursor_base_x = border.x/2.f - bar_width / 2.f;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glClearColor(BACKGROUND);
	glClear(GL_COLOR_BUFFER_BIT);


	renderer.image = font_tex;
	renderer_set_shader(&renderer, SHADER_FOR_TEXT);
	
	float time;
	char buffer[1024];
	
	player_get_duration(&player, &time);
	if(snprintf(buffer, 1023, "%.2f", time) >= 1023) {
	    panic("buffer overflow");
	}	
	render_line(&renderer,
		    0,
		    border.y - (float) font.height * 2.f,
		    &font, buffer);

	player_get_timestamp(&player, &time);
	if(snprintf(buffer, 1023, "%.2f", time) >= 1023) {
	    panic("buffer overflow");
	}
	render_line(&renderer,
		    border.x - (float) font_estimate_width(&font, buffer),
		    border.y - (float) font.height * 2.f,
		    &font, buffer);	
	
	renderer_flush(&renderer);

	renderer_set_shader(&renderer, SHADER_FOR_COLOR);
	//PLAY N PAUSE
	Vec2f pos = vec2f(border.x*.5f - button_width*.5f, 4 + button_width*.5f);
	renderer_solid_rect(&renderer,
			    pos,
			    vec2f(button_width, button_width),
			    FOREGROUND);
	float mousex = (float) event.mousex;
	float mousey = (float) event.mousey;
	if(click &&
	   mousex > pos.x &&
	   mousey > pos.y &&
	   mousex - pos.x < button_width &&
	   mousey - pos.y < button_width) {
	    player_toggle(&player);
	}
	
	//BAR
	renderer_solid_rect(&renderer,
			    vec2f(cursor_base_x, 68),
			    vec2f(bar_width, 8),
			    WHITE);

	//CURSOR
	float cursor_pos = (float) cursor_base_x;
	if(player.decoder_used) {
	    float timestamp, duration;
	    player_get_timestamp_secs(&player, &timestamp);
	    player_get_duration_secs(&player, &duration);

	    cursor_pos += (float) bar_width * timestamp / duration;
	}
	renderer_solid_rect(&renderer, vec2f(cursor_pos, 64), vec2f(16, 16), FOREGROUND);
	renderer_flush(&renderer);

	gui_swap_buffers(&gui);
    }

    //gui_free(&gui);

    return 0;
}

