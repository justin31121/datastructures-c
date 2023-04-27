#define URL "https://rr3---sn-8xgn5uxa-quhz.googlevideo.com/videoplayback?expire=1682614219&ei=a1NKZOLLN5uf-gaG-KeADg&ip=157.90.242.21&id=o-AH1LE1QpsjROpIotNiB-zd1fXdukVOOATskNWg-GS23r&itag=140&source=youtube&requiressl=yes&pcm2=yes&gcr=de&vprv=1&mime=audio%2Fmp4&ns=6C5hE4FIxZPz0GVHZcEM3zwN&gir=yes&clen=2795488&dur=172.686&lmt=1681229456935946&keepalive=yes&fexp=24007246&c=WEB&txp=4532434&n=_UcF8K1gaVfwgS-bcRU&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cpcm2%2Cgcr%2Cvprv%2Cmime%2Cns%2Cgir%2Cclen%2Cdur%2Clmt&ratebypass=yes&sig=AOq0QJ8wRQIgYpxyA2D9ZiGfEglThZjmBb34PWj_1IEcjCFZTzrCaz4CIQDoQBcJHG-85h733gl60GyrNlI9G8DSbYA13fdWhQO60A%3D%3D&redirect_counter=1&rm=sn-4g5eye76&req_id=7040214f938ba3ee&cms_redirect=yes&cmsv=e&ipbypass=yes&mh=xe&mip=2001:16b8:1809:2c00:10b5:832:5611:7382&mm=31&mn=sn-8xgn5uxa-quhz&ms=au&mt=1682592312&mv=m&mvi=3&pl=40&lsparams=ipbypass,mh,mip,mm,mn,ms,mv,mvi,pl&lsig=AG3C_xAwRQIhAKwynOxhajkeMbLShrQ8_75cme0PeNimVf9qBvj5ADlVAiBuIj-cjN5XXutTE5kO0xA0kwZRardmEqG9S7i7Cm895A%3D%3D"

#define PLAYER_IMPLEMENTATION
#include "../libs/player.h"

#define GUI_IMPLEMENTATION
#define GUI_OPENGL
#define GUI_CONSOLE
#include "../libs/gui.h"

#define RENDERER_IMPLEMENTATION
#include "../libs/renderer.h"

#define FONT_IMPLEMENTATION
#include "../libs/font.h"

#define IO_IMPLEMENTATION
#include "../libs/io.h"

#define DOWNLOAD_IMPLEMENTATION
#include "../libs/download.h"

#define HTML_PARSER_IMPLEMENTATION
#include "../libs/html_parser.h"

#include "../rsc/atlas.h"
#include "../rsc/musik.h"
#include "../rsc/segoeui.h"

#define BACKGROUND 0.09412, 0.09412, 0.09412, 1.0
#define FOREGROUND vec4f(0.451, 0.6392, 0.8 , 1) //0xFFCCA37e
#define WHITE vec4f(0.8667, 0.8667, 0.8667, 1)
#define WHITE_GREY vec4f(0.8, 0.8, 0.8, 1)
#define GREY vec4f(0.15, 0.15, 0.15, 1)

#define WIDTH 320
#define HEIGHT 344

void render_line(Renderer *renderer,
		 float x, float y,
		 Font2 *font, const char *word, Vec4f color) {
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
		  color);
    word_x += char_size.x;
  }
}

void load_files(const char *dir_path, Player *player, String_Buffer *buffer) {

  Io_Dir dir;
  if(!io_dir_open(&dir, dir_path)) {
    panic("io_dir_open");
  }

  Io_File file;
  while(io_dir_next(&dir, &file)) {
    if(file.is_dir) continue;    
    if(!player_open_file(player, file.abs_name)) {      
      continue;
    }
    string_buffer_append(buffer, file.abs_name, strlen(file.abs_name)+1);
    player_close(player);
  }

  io_dir_close(&dir);
}

#ifdef linux

#define YOUTUBE_IMPLEMENTATION
#include "../libs/youtube.h"

String_Buffer sbs[3] = {0};
duk_context *duk_ctx = NULL;

bool query_url(const char *videoId, char **url) {

  if(duk_ctx == NULL) {
    duk_ctx = duk_create_heap_default();
  }
  
  Http http;
  const char* hostname = "www.youtube.com";
  if(!http_init2(&http, hostname, strlen(hostname), true)) {
    return false;
  }
    
  Json out;
  if(!youtube_video(&http, sbs, NULL, duk_ctx, videoId, &out)) {
    return false;
  }  

  bool result = false;
  Json formats = json_get(out, "formats");
  for(s32 j=0;j<json_size(formats);j++) {
    Json format = json_opt(formats, j);
    int itag = json_get_int(format, "itag");
    if(itag == 140) {
      
      *url = json_get_string(format, "url");
      json_put_null(format, "url");
      
      result = true;
      break;
    }
  }

  json_free_all(out);
  http_free(&http);
  
  return result;
}

#endif //_linux

String_Buffer temp = {0};
String_Buffer filebuffer = {0};
String_Buffer sbuffer = {0};

int main(int argc, char **argv) {

  const char *dir_path = "./rsc/";
  if(argc > 1) {
    dir_path = argv[1];
  }
  int dir_path_len = strlen(dir_path);

  Gui gui;
  Gui_Canvas canvas = {WIDTH, HEIGHT, NULL};
  if(!gui_init(&gui, &canvas, "JPlayer")) {
    return -1;
  }
  gui_use_vsync(1);

  Renderer renderer = {0};
  if(!renderer_init(&renderer)) {
    return -1;
  }

  Font2 font = font2_segoeui_24;
  
  u32 font_tex = renderer_push_texture(&renderer, font.width, font.height, (char *) font.data, true);
  u32 atlas_tex = renderer_push_texture(&renderer, atlas_width, atlas_height, (char *) atlas_data, false);
  u32 musik_tex = renderer_push_texture(&renderer, musik_width, musik_height, (char *) musik_data, false);
  
  Player player;
  if(!player_init(&player, DECODER_FMT_S16, 2, 44100)) {
    panic("player_init");
  }

  load_files(dir_path, &player, &sbuffer);
  if(sbuffer.len == 0) { //no files found
    panic("load_files");
  }
  int sbuffer_pos = 0;

  bool playing_url = true;
  
#ifdef linux
  char *url;
  if(!query_url("7D89nNqLqQE", &url)) {
    panic("query_url");
  }
  for(int i=0;i<3;i++) string_buffer_free(&sbs[i]);
  duk_destroy_heap(duk_ctx);
#endif //linux
  
  if(!player_open_url(&player, URL)) {
    panic("player_open_url");
  }
  
  if(!player_play(&player)) {
    panic("player_play");
  }
  player_set_volume(&player, 0.1);

  float button_width = 24.f;
  float bar_y = 60.f;
  bool drag = false;
  bool was_paused = false;
    
  float bar_width = 0;
  float cursor_base_x = 0.f;
  float bar_margin = 40;

  Gui_Time time;
  gui_time_capture(&time);
  u32 acc = 0;
    
  Vec2f border;
  Gui_Event event;
  while(gui.running) {
    bool click = false;
    temp.len = 0;

    u32 ms = (u32) gui_time_measure(&time);
    if(player.playing) acc += ms;
	
    while(gui_peek(&gui, &event)) {
      if(event.type == GUI_EVENT_KEYPRESS) {
	switch(event.key) {
	case 'Q': {
	  gui.running = false;
	} break;
	case 'K': {		  
	  player_toggle(&player);
	} break;
	}

      } else if(event.type == GUI_EVENT_MOUSEPRESS) {
	if(event.key == 'L') {
	  click = true;
	}
      } else if(event.type == GUI_EVENT_MOUSERELEASE) {

	if(drag) {
	  drag = false;
		
	  float x = (float) event.mousex - 8.f;
	  if(x < cursor_base_x) {
	    x = cursor_base_x;
	  }
	  if(x + 16 >= cursor_base_x + bar_width) {
	    x = cursor_base_x + bar_width - 16;
	  }

	  if(player.decoder_used) {
	    float _time = (x - cursor_base_x) * player.duration_abs / (bar_width - 8.f);
		
	    player_seek(&player, _time);
	    if(!was_paused) {
	      player_play(&player); 
	    }		  
	  }
		
	}
      }
    }

    if(player.playing) {
      float d, n;
      player_get_timestamp(&player, &n);
      player_get_duration(&player, &d);

      if(d - n < 0.02 && n < d) {
	if(playing_url) {
	  playing_url = false;
	} else {
	  sbuffer_pos += strlen(sbuffer.data + sbuffer_pos) + 1;
	  if((size_t) sbuffer_pos >= sbuffer.len) sbuffer_pos = 0; 
	}

	player_close(&player);
	if(!player_open_file(&player, sbuffer.data + sbuffer_pos)) {
	  panic("player_open_file");
	}
	if(!player_play(&player)) {
	  panic("player_play");
	}
      }      
    }
	
    gui_get_window_sizef(&gui, &border.x, &border.y);
    bar_width = border.x * 0.8f;

    float mousex = (float) event.mousex;
    float mousey = (float) event.mousey;
    gui_mouse_to_screenf(border.x, border.y, &mousex, &mousey);
    bar_width = border.x - 2 * bar_margin;
    cursor_base_x = border.x/2.f - bar_width / 2.f;

    bool draw_files = border.x >= 2 * musik_width;
    float draw_files_height, draw_files_x, draw_files_y, draw_files_width;
    
    Vec2f pos = vec2f(WIDTH/2 - button_width/2, 4 + button_width*.5f);
    if(draw_files) {
      draw_files_x = pos.x + musik_width/2 + 2 * button_width;
      draw_files_width = border.x - draw_files_x - bar_margin;
      draw_files_height = border.y - bar_y - 2 * bar_margin;
      draw_files_y = border.y - draw_files_height;
    }

    renderer.time = (float) acc / 1000.f;
    renderer.resolution = border;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glClearColor(BACKGROUND);
    glClear(GL_COLOR_BUFFER_BIT);

    renderer_set_shader(&renderer, SHADER_FOR_COLOR);

    //FILES_RECT
    if(draw_files) {
      renderer_solid_rect(&renderer,
			  vec2f(draw_files_x, draw_files_y),
			  vec2f(draw_files_width, draw_files_height - 1 * bar_margin),
			  GREY);          
    }
    
    //BAR
    if(click &&
       mousex >= cursor_base_x &&
       mousey >= bar_y &&
       mousex - cursor_base_x < bar_width &&
       mousey - bar_y < 8) {	  

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
			vec2f(cursor_base_x, bar_y),
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
	   mousey > (bar_y - 4) &&
	   mousex - cursor_pos < 16 &&
	   mousey - (bar_y - 4) < 16) {
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

    renderer_solid_rect(&renderer, vec2f(cursor_pos, bar_y - 4), vec2f(16, 16), FOREGROUND);
    renderer_flush(&renderer);

    renderer.image = font_tex;
    renderer_set_shader(&renderer, SHADER_FOR_TEXT);

    //TITLE
    if(!playing_url) {
      const char *text = tprintf(&temp, "%s", sbuffer.data + sbuffer_pos + dir_path_len);    
      render_line(&renderer,
		  0, border.y - (float) font.height,
		  &font, text, vec4f(1, 1, 1, 1));
    }

    //TIME
    float _time;
    if(drag) {
      float x = (float) event.mousex - 8.f;
      if(x < cursor_base_x) {
	x = cursor_base_x;
      }
      if(x + 16>= cursor_base_x + bar_width) {
	x = cursor_base_x + bar_width - 16;
      }

      if(player.decoder_used) {
	float secs = (x - cursor_base_x) * player.duration_abs / (bar_width - 8.f);
	_time = floorf(secs / 60.f);
	_time += (float) ((int) secs % 60) / 100;	    
      }
    } else {
      player_get_timestamp(&player, &_time);
    }

    //DRAW_FILES - FILES
    if(draw_files) {
      float file_y = draw_files_y + draw_files_height - 1 * bar_margin - (float) font.height;
      size_t off = 0;
      while(off < sbuffer.len) {
	float len = font_estimate_width2(&font, sbuffer.data + dir_path_len + off);
	if(click &&
	   mousex > draw_files_x &&
	   mousey > file_y &&
	   mousex - draw_files_x < len &&
	   mousey - file_y < (float) font.height) {

	  sbuffer_pos = off;
	  
	  player_close(&player);
	  if(!player_open_file(&player, sbuffer.data + sbuffer_pos)) {
	    panic("player_open_file");
	  }
	  if(!player_play(&player)) {
	    panic("player_play");
	  }

	  
	}
	
	render_line(&renderer, draw_files_x, file_y, &font, sbuffer.data + dir_path_len + off, (size_t) sbuffer_pos == off ? FOREGROUND : WHITE);
	file_y -= (float) font.height;
	off += strlen(sbuffer.data + off)+1;
      }

    }

    const char *text = tprintf(&temp, "%.2f", _time);
    float text_width = (float) font_estimate_width2(&font, text);

    render_line(&renderer,
		bar_margin - text_width / 2, bar_y + (float) font.height,
		&font, text, vec4f(1, 1, 1, 1));

    player_get_duration(&player, &_time);
    text = tprintf(&temp, "%.2f", _time);
    text_width = (float) font_estimate_width2(&font, text);	
	
    render_line(&renderer,
		border.x - bar_margin - .5f * text_width,
		bar_y + (float) font.height,
		&font, text, vec4f(1, 1, 1, 1));
    renderer_flush(&renderer);
	
	
    renderer.image = atlas_tex;
    renderer_set_shader(&renderer, SHADER_FOR_IMAGE);

    //PLAY N PAUSE
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
    Vec2f _pos = vec2f_sub(pos, vec2f(2*button_width, 0));
    if(click &&
       mousex > _pos.x &&
       mousey > _pos.y &&
       mousex - _pos.x < button_width &&
       mousey - _pos.y < button_width) {

      sbuffer_pos -= 2;
      if(sbuffer_pos < 0) {
	sbuffer_pos = sbuffer.len - 1;
      }
      while(sbuffer_pos > 0 &&
	    sbuffer.data[sbuffer_pos - 1] != 0) {
	sbuffer_pos--;
      }
	  
      player_close(&player);
      if(!player_open_file(&player, sbuffer.data + sbuffer_pos)) {
	panic("player_open_file");
      }
      if(!player_play(&player)) {
	panic("player_play");
      }
	  
    }	
    renderer_image_rect(&renderer, _pos, vec2f(button_width,  button_width), vec2f(.5, 0), vec2f(.25, 1));

    //NEXT
    _pos = vec2f_add(pos, vec2f(2*button_width, 0));
    if(click &&
       mousex > _pos.x &&
       mousey > _pos.y &&
       mousex - _pos.x < button_width &&
       mousey - _pos.y < button_width) {

      if(playing_url) {
	playing_url = false;
      } else {
	sbuffer_pos += strlen(sbuffer.data + sbuffer_pos) + 1;
	if((size_t) sbuffer_pos >= sbuffer.len) sbuffer_pos = 0;	      
      }

      if(player.decoder_used) {
	player_close(&player);
	if(!player_open_file(&player, sbuffer.data + sbuffer_pos)) {
	  panic("player_open_file");
	}
	if(!player_play(&player)) {
	  panic("player_play");
	}
      }

    }	
    renderer_image_rect(&renderer, _pos, vec2f(button_width,  button_width), vec2f(.75, 0), vec2f(.25, 1));
	
    renderer_flush(&renderer);

    renderer.image = musik_tex;
    renderer_set_shader(&renderer, SHADER_FOR_RIPPLE);
    //LOGO - MUSIK
    renderer_image_rect(&renderer,
			vec2f(pos.x - musik_width / 2, bar_y + 1.5 * button_width),
			vec2f(musik_width, musik_height), vec2f(0, 0), vec2f(1, 1));
    renderer_flush(&renderer);
	
    gui_swap_buffers(&gui);
  }

  //gui_free(&gui);

  return 0;
}

