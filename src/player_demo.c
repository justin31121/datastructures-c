#define PLAYER_IMPLEMENTATION
#include "../libs/player.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../thirdparty/stb_truetype.h"

#define GUI_OPENGL
//#define GUI_LOOPLESS
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

#define FOREGROUND vec4f(0.451, 0.6392, 0.8 , 1)
#define WHITE vec4f(0.8667, 0.8667, 0.8667, 1)
#define GREY vec4f(0.15, 0.15, 0.15, 1)

#include "../rsc/musik.h"
#include "../rsc/atlas.h"
#include "../rsc/segoeui.h"

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
    return NULL;
  }

  loading_log = "opening song . . .";
  if(!player_open(&player, playlist_get_source(&playlist, playlist.pos))) {
    return NULL;
  }

  loading_log = "start playing . . .";
  if(!player_play(&player)) {
    return NULL;
  }
  
  loading = false;
  loading_log = NULL;

  return NULL;
}

int main(int argc, char **argv) {
  av_log_set_level(AV_LOG_QUIET);

  Gui gui;
  //Gui_Canvas canvas = {WIDTH, HEIGHT, NULL};
  Gui_Canvas canvas = {WIDTH*2, HEIGHT, NULL};
  Gui_Event event;
  if(!gui_init(&gui, &canvas, "JPlayer")) {
    return 1;
  }
  gui_use_vsync(1);

  if(!imgui_init(&gui, &event)) {
    return 1;
  }

  char *arg = ".\\rsc\\";
  if(argc > 1) {
    arg = argv[1];
  }

  Thread player_start_thread;
  if(!thread_create(&player_start_thread, player_start, arg)) {
    return 1;
  }

  Font2 font = font2_segoeui_24;
  imgui_set_font(&font);
  int musik = imgui_add_img(musik_data, musik_width, musik_height);
  int atlas = imgui_add_img(atlas_data, atlas_width, atlas_height);
  imgui_set_background(0xff181818);
  
  Imgui_ListView listView = {
    .arg = &playlist,
    .pos = playlist.pos,
    .scroll_pos = 0.f,
    .off = -1.f,
    .len = playlist.len,
    .get = playlist_get_name,
    .enabled = playlist_is_enabled,
  };

  float width, height;
  float slider, secs = 0.f;
  float volume = 0.f;
  while(gui.running) {
    temp.len = 0;
    while(imgui_peek()) {
      if(event.type == GUI_EVENT_KEYPRESS) {
	switch(event.key) {
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
	}
      }
    }
    if(player.playing) {
      float d = player.duration;
      float n;
      player_get_timestamp(&player, &n);
      if(d - n < 0.02 && n < d) {
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
    bool draw_files = width >= 2 * musik_width;
    Vec2f draw_files_pos = vec2f(WIDTH/2 + musik_width/2, BAR_Y + 2*BAR_MARGIN);
    Vec2f draw_files_size = vec2f(width - WIDTH/2 - musik_width/2 - BAR_MARGIN,
				  height - BAR_Y - 3*BAR_MARGIN);
    if(draw_files) {
      imgui_rect(draw_files_pos, draw_files_size, GREY);
    }    
    //VOLUME BAR
    volume = 0.f;
    if(player.decoder_used) {
      volume = player.decoder.volume;
    }
    imgui_tribar(vec2f(width - BUTTON_WIDTH/2 - TRIBAR_WIDTH, BUTTON_WIDTH/2),
		 vec2f(TRIBAR_WIDTH, atlas_height),
		 FOREGROUND, GREY, volume, &volume);
    if(player.decoder.volume + EPSILON < volume ||
       player.decoder.volume + EPSILON > volume) {
      player_set_volume(&player, volume); 
    }
    //FILES
    if(!loading && draw_files) {
      listView.pos = playlist.pos;
      listView.len = playlist.len;
      if(playlist.available == playlist.len && playlist.using_yt_context) {
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
      const char *text = tprintf(&temp, "%.2f", slider * player.duration);
      float text_width = font_estimate_width2(&font, text);
      imgui_text(vec2f(BAR_MARGIN - text_width/2, BAR_Y + font.height), text, WHITE);

      text = tprintf(&temp, "%.2f", player.duration);
      text_width = (float) font_estimate_width2(&font, text);
      imgui_text(vec2f(width - BAR_MARGIN - text_width/2, BAR_Y + font.height), text, WHITE);
    }
    //NAME
    if(loading) {
      char *text = loading_log == NULL ? "loading . . . " : loading_log;
      imgui_text(vec2f(0, height - font.height), text, WHITE);
    } else {
      imgui_text(vec2f(0, height - font.height), playlist_get_name(&playlist, playlist.pos), WHITE);      
    }
    //LOGO
    imgui_img(musik, vec2f(WIDTH/2 - BUTTON_WIDTH/2 - musik_width/2, BAR_Y + 1.5*BUTTON_WIDTH),
	      vec2f(musik_width, musik_height));
    if(imgui_subimg_button(atlas, vec2f(WIDTH/2 - BUTTON_WIDTH/2, BUTTON_WIDTH/2),
			   vec2f(BUTTON_WIDTH, BUTTON_WIDTH),
			   vec2f(player.playing ? .25 : 0, 0), vec2f(.25, 1))) {
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
