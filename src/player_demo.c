#define PLAYER_IMPLEMENTATION
#include "../libs/player.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../thirdparty/stb_truetype.h"

#define GUI_OPENGL
#define IMGUI_RENDERER_IMPLEMENTATION
#include "../libs/imgui.h"

#define PLAYLIST_IMPLEMENTATION
#include "../libs/playlist.h"

#define EPSILON 0.001

#define WIDTH 320
#define HEIGHT 344
#define BUTTON_WIDTH 24

#define BAR_Y 60
#define BAR_HEIGHT 8
#define BAR_MARGIN 40

#define TRIBAR_WIDTH 64

#include "../rsc/musik.h"
#include "../rsc/atlas.h"

static String_Buffer temp = {0};
static Player player;
static Playlist playlist;
static bool loading = true;
static char *loading_log = NULL;

void toggle() {
  player_toggle(&player);
}

void prev() {
  if(!player.decoder_used) return;
  player_close(&player);
  playlist_prev(&playlist);
  player_open(&player, playlist_get_source(&playlist, playlist.pos));
  player_play(&player);  
}

void next() {
  if(!player.decoder_used) return;
  player_close(&player);
  playlist_next(&playlist);
  player_open(&player, playlist_get_source(&playlist, playlist.pos));
  player_play(&player);
}

void choose(size_t pos) {
  if(!player.decoder_used) return;
  if(playlist.pos == pos) return;
  player_close(&player);
  playlist.pos = pos;
  player_open(&player, playlist_get_source(&playlist, playlist.pos));
  player_play(&player);
}

void *player_start(void *_arg) {
  char *arg = (char *) _arg;

  loading_log = "initializing player . . .";
  if(!player_init(&player, DECODER_FMT_S16, 2, 44100)) {
    return NULL;
  }

  loading_log = "initializing playlist . . .";
  playlist_init(&playlist);
  if(!playlist_from(&playlist, &player, arg)) {
    fprintf(stderr, "ERROR: Can not play argument: '%s'\n", arg);
    panic("playlist_from");
  }

  loading_log = "opening song . . .";
  if(!player_open(&player, playlist_get_source(&playlist, playlist.pos))) {
    panic("player_open");
  }

  loading_log = "start playing . . .";
  if(!player_play(&player)) {
    panic("player_play");
  }
  
  loading = false;
  loading_log = NULL;

  return NULL;
}

const char *list_get(void *arg, size_t pos) {
    return playlist_get_name((Playlist *) arg, pos);
}

bool list_is_enabled(void *arg, size_t pos) {
    return playlist_is_enabled((Playlist *) arg, pos);
}

int main(int argc, char **argv) {
  av_log_set_level(AV_LOG_QUIET);

  Gui gui;
  //Gui_Canvas canvas = {WIDTH, HEIGHT, NULL};
  Gui_Canvas canvas = {WIDTH, HEIGHT, NULL};
  Gui_Event event;
  if(!gui_init(&gui, &canvas, "JPlayer")) {
    return 1;
  }
  gui_use_vsync(1);

  if(!imgui_init(&gui, &event)) {
    return 1;
  }

  char *arg = "./rsc/";
  if(argc > 1) {
    arg = argv[1];
  }

  Thread player_start_thread;
  if(!thread_create(&player_start_thread, player_start, arg)) {
    return 1;
  }

#include "../rsc/segoeui.h"
  Font2 font = font2_segoeui_24; 
  imgui_set_font(&font);
  
  int musik = imgui_add_img(musik_data, musik_width, musik_height);
  int atlas = imgui_add_img(atlas_data, atlas_width, atlas_height);
  imgui_set_background(0xff181818);

  Vec4f FOREGROUND = vec4f(0.451f, 0.6392f, 0.8f, 1.f);
  Vec4f WHITE      = vec4f(0.8667f, 0.8667f, 0.8667f, 1.f);
  Vec4f GREY       = vec4f(0.15f, 0.15f, 0.15f, 1.f);
  
  Imgui_ListView listView = {
    .arg = &playlist,
    .pos = playlist.pos,
    .scroll_pos = 0.f,
    .off = -1.f,
    .len = playlist.len,
    .get = list_get,
    .enabled = list_is_enabled,
  };

  float width, height;
  float slider, secs = 0.f;
  float volume = .1f;
  bool holding_w = false;
  bool holding_s = false;
  while(gui.running) {
    temp.len = 0;
    if(player.decoder_used) {
      volume = player.current_volume;
    }
    while(imgui_peek()) {
      if(event.type == GUI_EVENT_KEYPRESS) {
	switch(event.as.key) {
	case 'A': {
	  prev();
	} break;
	case 'D': {
	  next();
	} break;
	case 'K': {
	  toggle();
	} break;
	case 'Q': {
	  gui.running = false;	  
	} break;
	case 'W': {
	  holding_w = true;
	} break;
	case 'S': {
	  holding_s = true;
	} break;
	}
      } else if(event.type == GUI_EVENT_KEYRELEASE) {
	switch(event.as.key) {
	case 'W': {
	  holding_w = false;
	} break;
	case 'S': {
	  holding_s = false;
	} break;
	}
      }
    }
    if(holding_w) {
      if(player.decoder_used) {
	volume += .0125f;
	if(volume > 1.f) volume = 1.f;
      }   
    }
    if(holding_s) {
      if(player.decoder_used) {
	volume -= .0125f;
	if(volume < 0.f) volume = 0.f;
      }	    
    }
    if(player.playing) {
      float d = player.duration_abs;
      float n;
      player_get_timestamp_abs(&player, &n);
      if(d - n < 1.f && n < d) {
	next();
      }
    }
    
    imgui_get_size(&width, &height);
    if(player.playing) {
      player_get_timestamp_abs(&player, &secs);
      secs /= player.duration_abs;
    }

    //BAR
    float bar_width = width - 2*BAR_MARGIN;
    if(imgui_bar(vec2f(width/2 - bar_width/2, BAR_Y), vec2f(bar_width, BAR_HEIGHT),
		 FOREGROUND, WHITE, secs, &slider)) {
      player_seek(&player, slider * player.duration_abs);
      player_play(&player);
      secs = slider;
    }
    if(slider != secs) {
      player_stop(&player);
    }
    //FILES - RECT
    bool draw_files = (int) width >= 2 * musik_width;
    Vec2f draw_files_pos = vec2f((float) (WIDTH/2 + musik_width/2), BAR_Y + 2*BAR_MARGIN);
    Vec2f draw_files_size = vec2f(width - WIDTH/2 - musik_width/2 - BAR_MARGIN,
				  height - BAR_Y - 3*BAR_MARGIN);
    if(draw_files) {
      imgui_rect(draw_files_pos, draw_files_size, GREY);
    }    
    //VOLUME BAR
    if(player.decoder.volume + EPSILON < volume ||
       player.decoder.volume + EPSILON > volume) {
      player_set_volume(&player, volume); 
    }	

    imgui_tribar(vec2f(width - BUTTON_WIDTH/2 - TRIBAR_WIDTH, BUTTON_WIDTH/2),
		 vec2f(TRIBAR_WIDTH, (float) atlas_height),
		 FOREGROUND, GREY, volume, &volume);
    if(player.decoder.volume + EPSILON < volume ||
       player.decoder.volume + EPSILON > volume) {
      player_set_volume(&player, volume); 
    }
    //FILES
    if(!loading && draw_files) {
      listView.pos = playlist.pos;
      listView.len = playlist.len;
      if(playlist.len != 0 &&
	 playlist.available == playlist.len &&
	 playlist.using_yt_context) {
	youtube_context_free(&playlist.yt_context);
	playlist.using_yt_context = false;
      }
      int pos;
      if((pos = imgui_list(draw_files_pos, draw_files_size, FOREGROUND, WHITE, &listView)) >= 0) {
	choose(pos);
      }
    }
    //TIME
    if(!loading) {      
      float seconds = slider * player.duration_abs;
      float minutes = (float) ((int) (seconds / 60.f) % 10);
      float fractional = roundf(seconds - minutes * 60.f) / 100.f;
      if(fractional == 0.60f) {
	fractional = 0.f;
	minutes += 1.f;
      }
      const char *text = tprintf(&temp, "%.2f", minutes + fractional);
      float text_width = (float) font_estimate_width2(&font, text);
      imgui_text(vec2f(BAR_MARGIN - text_width/2, (float) (BAR_Y + font.height)), text, WHITE);

      text = tprintf(&temp, "%.2f", player.duration);
      text_width = (float) font_estimate_width2(&font, text);
      imgui_text(vec2f(width - BAR_MARGIN - text_width/2, (float) (BAR_Y + font.height)), text, WHITE);
    }
    //NAME
    if(loading) {
      char *text = loading_log == NULL ? "loading . . . " : loading_log;
      imgui_text(vec2f(0, height - font.height), text, WHITE);
    } else {
      imgui_text(vec2f(0, height - font.height), playlist_get_name(&playlist, playlist.pos), WHITE);      
    }
    //LOGO
    imgui_img(musik, vec2f((float) (WIDTH/2 - BUTTON_WIDTH/2 - musik_width/2), BAR_Y + 1.5*BUTTON_WIDTH),
	      vec2f((float) musik_width, (float) musik_height));
    if(imgui_subimg_button(atlas, vec2f(WIDTH/2 - BUTTON_WIDTH/2, BUTTON_WIDTH/2),
			   vec2f(BUTTON_WIDTH, BUTTON_WIDTH),
			   vec2f(player.playing ? .25f : 0.f, 0.f), vec2f(.25f, 1.f))) {
      toggle();
    }
    if(imgui_subimg_button(atlas, vec2f(WIDTH/2 - BUTTON_WIDTH/2 + 2*BUTTON_WIDTH, BUTTON_WIDTH/2),
			   vec2f(BUTTON_WIDTH, BUTTON_WIDTH),
			   vec2f(.75, 0), vec2f(.25, 1))) {
      next();
    }
    if(imgui_subimg_button(atlas, vec2f(WIDTH/2 - BUTTON_WIDTH/2 - 2*BUTTON_WIDTH, BUTTON_WIDTH/2),
			   vec2f(BUTTON_WIDTH, BUTTON_WIDTH),
			   vec2f(.5, 0), vec2f(.25, 1))) {
      prev();
    }
    
    imgui_update();
  }

  //player_free(&player);
  
  return 0;
}
