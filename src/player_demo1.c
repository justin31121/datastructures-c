#define PLAYER_IMPLEMENTATION
#include "../libs/player.h"

#define GUI_IMPLEMENTATION
#define GUI_OPENGL
#include "../libs/gui.h"

#define RENDERER_IMPLEMENTATION
#include "../libs/renderer.h"

#define FONT_IMPLEMENTATION
#include "../libs/font.h"

#define IO_IMPLEMENTATION
#include "../libs/io.h"

#define HTML_PARSER_IMPLEMENTATION
#include "../libs/html_parser.h"

#define SPOTIFY_IMPLEMENTATION
#include "../libs/spotify.h"

#define YOUTUBE_IMPLEMENTATION
#include "../libs/youtube.h"

#include "../rsc/atlas.h"
#include "../rsc/musik.h"
#include "../rsc/segoeui.h"

#define BACKGROUND 0.09412, 0.09412, 0.09412, 1.0
#define FOREGROUND vec4f(0.451, 0.6392, 0.8 , 1) //0xFFCCA37e
#define RED vec4f(1, 0, 0, 1)
#define WHITE vec4f(0.8667, 0.8667, 0.8667, 1)
#define GREY_WHITE vec4f(.275, .275, .275, 1)
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

String_Buffer sbs[3] = {0};
duk_context *duk_ctx = NULL;
Http youtube_http;

bool query_url_by_videoId(string videoId, char **url) {

    if(duk_ctx == NULL) {
	duk_ctx = duk_create_heap_default();
	const char* hostname = "www.youtube.com";
	if(!http_init2(&youtube_http, hostname, strlen(hostname), true)) {
	    return false;
	}
    }

    HttpHeader header;
    bool url_is_valid = false;

    for(int i=0;i<3;i++) {
	Json out;
	if(!youtube_video2(&youtube_http, sbs, NULL, duk_ctx, videoId, &out)) {
	    return false;
	}

	Json formats = json_get(out, "formats");
	for(s32 j=0;j<json_size(formats);j++) {
	    Json format = json_opt(formats, j);
	    int itag = json_get_int(format, "itag");
	    if(itag == 140) {      
		*url = json_get_string(format, "url");

		if(!http_head((const char *) *url, &header, NULL)) {
		    break;
		}
		s32 responseCode = http_header_response_code(&header);
		url_is_valid = responseCode == 200;

		if(url_is_valid) {
		    json_put_null(format, "url");
		}
		break;
	    }
	}
    
	json_free_all(out);
	if(url_is_valid) break;
    }

    return url_is_valid;
}

bool query_url_by_keyword(const char *keyword, char **url) {

    if(duk_ctx == NULL) {
	duk_ctx = duk_create_heap_default();
	const char* hostname = "www.youtube.com";
	if(!http_init2(&youtube_http, hostname, strlen(hostname), true)) {
	    return false;
	}
    }
    
    Json results;
    if(!youtube_search(&youtube_http, keyword, &results)) {
	return false;
    }

    Json result = json_opt(results, 0);
    const char* videoId = json_get_string(result, "videoId");

    if(!query_url_by_videoId(string_from_cstr(videoId), url)) {
	return false;
    }
  
    json_free_all(results);

    return true;
}

bool get_videoId(const char *url, string *videoId) {
    string id = {0};

    string url_string = string_from_cstr(url);

    if(!string_chop_string(&url_string, STRING("https://www.youtube.com/watch?"))) {
	return false;
    }
  
    while(url_string.len) {    
	string pair = string_chop_by_delim(&url_string, '&');
	string value = string_chop_by_delim(&pair, '=');
	if(string_eq(value, STRING("v"))) {
	    id = pair;
	}
    }

    if(!id.len) {
	return false;
    }

    *videoId = id;
    return true;
}

typedef struct{
    float x, y, w, h;
}Duration_Bar;

typedef enum{
    STATE_FILE,
    STATE_DIR,
    STATE_URL,
    STATE_YOUTUBE,
    STATE_SPOTIFY_TRACK,
    STATE_SPOTIFY,
}State;

String_Buffer temp = {0};
String_Buffer dir_buffer = {0};
String_Buffer payload_buffer = {0};

static Player player;
static Gui gui;

#define YOUTUBE_PREFIX "https://www.youtube.com/watch?"
#define SPOTIFY_TRACK_PREFIX "https://open.spotify.com/track/"
#define SPOTIFY_PREFIX "https://open.spotify.com/"

char *url = NULL;

const char *path = "./rsc/";
int dir_path_len = 0;

int dir_buffer_pos = 0;

int keyword_buffer_pos = 0;

int dir_queried_pos = 0;
int payload_buffer_pos = 0;

void *query_thread_function(void *arg) {
    (void) arg;
  
    while((size_t) dir_queried_pos < dir_buffer.len) {
	if(!query_url_by_keyword(dir_buffer.data + dir_queried_pos, &url)) {
	    fprintf(stderr, "ERROR: Could not query: '%s'\n", dir_buffer.data + dir_queried_pos);
	    break;
	}
	string_buffer_append(&payload_buffer, url, strlen(url) + 1);
	free(url);
	dir_queried_pos += strlen(dir_buffer.data + dir_queried_pos) + 1;
    }

    for(int i=0;i<3;i++) string_buffer_free(&sbs[i]);
    duk_destroy_heap(duk_ctx);
    http_free(&youtube_http);
  
    return NULL;
}

void play(State state) {
  
    switch(state) {
    case STATE_FILE: {
	if(!player_open_file(&player, path)) {
	    panic("player_open_file");
	}
    } break;
    case STATE_DIR: {
	dir_path_len = strlen(path);
    
	load_files(path, &player, &dir_buffer);
	if(dir_buffer.len == 0) {
	    panic("No files found in dir: '%s'\n", path);
	}

	if(!player_open_file(&player, dir_buffer.data)) {
	    panic("player_open_file");
	}
    } break;
    case STATE_URL: {
	if(!player_open_url(&player, path)) {
	    panic("player_open_url");
	}
    } break;
    case STATE_YOUTUBE: {
	if(url != NULL) {
	    if(!player_open_url(&player, url)) {
		url = NULL;
		play(state);
	    }
	    return;
	}
    
	string videoId;
	if(!get_videoId(path, &videoId)) {
	    panic("Could not parse the videoId");
	}
    
	if(!query_url_by_videoId(videoId, &url)) {
	    panic("Could not resolve the videoId: '"String_Fmt"'", String_Arg(videoId));
	}

	if(!player_open_url(&player, url)) {
	  printf("%s\n", url);
	  panic("player_open_url");
	}

	for(int i=0;i<3;i++) string_buffer_free(&sbs[i]);
	duk_destroy_heap(duk_ctx);
	http_free(&youtube_http);
    } break;
    case STATE_SPOTIFY_TRACK: {
	if(url != NULL) {
	    if(!player_open_url(&player, url)) {
		url = NULL;
		play(state);
	    }
	    return;
	}
    
	const char *trackId = path + strlen(SPOTIFY_TRACK_PREFIX);

	Json track;
	if(!spotify_track_by_id(trackId, &track)) {
	    panic("spotify_track_by_id");
	}

	char keyword[1024];
	if(!spotify_get_keyword(track, keyword)) {
	    panic("spotify_get_keyword");
	}
	json_free_all(track);

	if(!query_url_by_keyword(keyword, &url)) {
	    panic("query_url_by_keyword");
	}

	if(!player_open_url(&player, url)) {
	  printf("%s\n", url);
	  panic("player_open_url");
	}

	for(int i=0;i<3;i++) string_buffer_free(&sbs[i]);
	duk_destroy_heap(duk_ctx);
	http_free(&youtube_http);
    } break;
    case STATE_SPOTIFY: {
	const char *off = path + strlen(SPOTIFY_PREFIX);
    
	const char *playlist_cstr = "playlist";
	size_t playlist_cstr_len = strlen(playlist_cstr);
    
	const char *album_cstr = "album";
	size_t album_cstr_len = strlen(album_cstr);

	Json spotify_tracks;
    
	int pos;
	if((pos = cstr_index_of(off, strlen(off), playlist_cstr, playlist_cstr_len)) == 0) {
	    if(!spotify_playlist_by_id(off + 1 + playlist_cstr_len, &spotify_tracks)) {
		panic("spotify_playlist_by_id");
	    }
	} else if((pos = cstr_index_of(off, strlen(off), album_cstr, strlen(album_cstr))) == 0) {
	    if(!spotify_album_by_id(off + 1 + album_cstr_len, &spotify_tracks)) {
		panic("spotify_album_by_id");
	    }      
	} else {
	    panic("The spotify url is invalid. Valid paths are 'playlist' and 'album'");
	}
    
	//TODO: This is a preventive mechanism
	//such that the url-pointer passed to player_open_url is not moved afterwards.
	//This does not ensure that the memory at payload_buffer.data is not moved, but makes it
	//unlikely
	string_buffer_reserve(&payload_buffer, 1024 * 4);

	char keyword[1024];
	Json tracks = json_get(json_get(spotify_tracks, "tracks"), "items");
	for(s32 i=0;i<json_size(tracks) && i<20;i++) {
	    Json track = json_opt(tracks, i);
	    if(json_has(track, "track")) {
		track = json_get(track, "track");
	    }

	    const char *name = json_get_string(track, "name");
	    if(!spotify_get_keyword(track, keyword)) {
		fprintf(stderr, "WARNING: Failed to build keyword for song: '%s'\n", name);
		continue;
	    }
      
	    string_buffer_append(&dir_buffer, keyword, strlen(keyword) + 1);
	}    
	json_free_all(spotify_tracks);

	if(!query_url_by_keyword(dir_buffer.data, &url)) {
	    panic("query_url_by_keyword");
	}
	string_buffer_append(&payload_buffer, url, strlen(url) + 1);
	free(url);
	dir_queried_pos += strlen(dir_buffer.data) + 1;

	Thread id;
	if(!thread_create(&id, query_thread_function, NULL)) {
	    panic("thread_create");
	}

	if(!player_open_url(&player, payload_buffer.data)) {
	    panic("player_open_url");
	}
    } break;
    default: {
	panic("unreachable state: %d\n", state);
    } break;
    }

}

int main(int argc, char **argv) {

    if(!player_init(&player, DECODER_FMT_S16, 2, 44100)) {
	panic("player_init");
    }

    State state = STATE_DIR;
  
    if(argc > 1) {
	char *arg = argv[1];
	path = arg;
    
	bool is_file;
	if(io_exists(arg, &is_file)) {
	    if(is_file) {
		state = STATE_FILE;
	    }
	} else {
	    int path_len = strlen(path);
      
	    int pos;
	    if((pos = cstr_index_of(path, path_len,
				    YOUTUBE_PREFIX, strlen(YOUTUBE_PREFIX))) == 0) {
		state = STATE_YOUTUBE;
	    } else if((pos = cstr_index_of(path, path_len,
					   SPOTIFY_TRACK_PREFIX, strlen(SPOTIFY_TRACK_PREFIX))) == 0) {
		state = STATE_SPOTIFY_TRACK;
	    } else if((pos = cstr_index_of(path, path_len,
					   SPOTIFY_PREFIX, strlen(SPOTIFY_PREFIX))) == 0) {
		state = STATE_SPOTIFY;
	    } else {
		state = STATE_URL;
	    }
      
	}
    }

    play(state);

    Gui_Canvas canvas = {WIDTH, HEIGHT, NULL};
    if(!gui_init(&gui, &canvas, "JPlayer")) {
	return -1;
    }
    gui_use_vsync(1);

    if(!player_play(&player)) {
	panic("player_play");
    }
    player_set_volume(&player, .1);

    Renderer renderer = {0};
    if(!renderer_init(&renderer)) {
	return -1;
    }

    Font2 font = font2_segoeui_24;
  
    u32 font_tex = renderer_push_texture(&renderer, font.width, font.height, (char *) font.data, true);
    u32 atlas_tex = renderer_push_texture(&renderer, atlas_width, atlas_height, (char *) atlas_data, false);
    u32 musik_tex = renderer_push_texture(&renderer, musik_width, musik_height, (char *) musik_data, false); 
  
    float button_width = 24.f;
    float bar_y = 60.f;
    bool drag = false;
    bool bar_drag = false;
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
		bar_drag = false;
		drag = false;	
	    }
	}
    
	gui_get_window_sizef(&gui, &border.x, &border.y);
	bar_width = border.x * 0.8f;

	float mousex = (float) event.mousex;
	float mousey = (float) event.mousey;
	gui_mouse_to_screenf(border.x, border.y, &mousex, &mousey);

	if(player.playing) {
	    float d, n;
	    player_get_timestamp(&player, &n);
	    player_get_duration(&player, &d);

	    if(d - n < 0.02 && n < d) {
		player_close(&player);

		if(state == STATE_DIR || state == STATE_SPOTIFY) {
		    dir_buffer_pos += strlen(dir_buffer.data + dir_buffer_pos) + 1;
		    if((size_t) dir_buffer_pos >= dir_buffer.len) dir_buffer_pos = 0;

		    if(state == STATE_SPOTIFY) {
			payload_buffer_pos += strlen(payload_buffer.data + payload_buffer_pos) + 1;
			if((size_t) payload_buffer_pos >= payload_buffer.len) payload_buffer_pos = 0;
		    }

		    if(state == STATE_DIR) {
			if(!player_open_file(&player, dir_buffer.data + dir_buffer_pos)) {
			    panic("player_open_file");
			}	    
		    } else {
			if(!player_open_url(&player, payload_buffer.data + payload_buffer_pos)) {
			    panic("player_open_url");
			}
		    }
	  
		    if(!player_play(&player)) {
			panic("player_play");
		    }

		}
	
	    }      
	}
    
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

	Duration_Bar bar = { 
	    .x = border.x - pos.y,
	    .y = pos.y,
	    .w = 64.f,
	    .h = (float) atlas_height,
	};

	if(bar_drag) {
	    float x = mousex;
	    float volume;
	    if(x < (bar.x - bar.w)) {
		volume = 0.0f;
	    } else if(x > bar.x) {
		volume = 1.0f;
	    } else {
		float _temp = x - (bar.x - bar.w);
		volume =  _temp / bar.w;
	    }

	    float epsilon = 0.001;

	    float volume_diff = player.decoder.volume - volume;
	    if(volume_diff < 0) {
		volume_diff *= - 1;
	    }

	    if(0.0f <= volume && volume <= 1.0f
	       && volume_diff > epsilon) {
		player_set_volume(&player, volume);
	    }
	}
    
	if(click &&
	   (bar.x - bar.w) <= mousex && mousex < (bar.x) &&
	   mousey <= (bar.y + bar.h) && mousey > (bar.y)) {
      
	    float px = (mousex - (bar.x - bar.w));
	    if((bar.y - (px * bar.h / bar.w)) < mousey) {
		//bar.on_bar_clicked((float) px / (float) bar.w, bar.arg);
		bar_drag = true;
		player_set_volume(&player, px / bar.w);
	    }
      
	}
    
	//TRIANGLE
	float x1 = bar.x,          y1 = bar.y;
	float x2 = bar.x,          y2 = bar.y + bar.h;
	float x3 = bar.x - bar.w,  y3 = bar.y;

	//  BACK
	renderer_solid_triangle(&renderer,
				vec2f(x1, y1), vec2f(x2, y2), vec2f(x3, y3), GREY);
   
	float dx = (float) ((1.0f - player.decoder.volume) * (float) bar.w);
	x1 -= dx;
	x2 -= dx;
  
	float dy = bar.h - ((bar.w - dx) * bar.h / bar.w);
	y2 -= dy;

	//  FRONT
	renderer_solid_triangle(&renderer,
				vec2f(x1, y1), vec2f(x2, y2), vec2f(x3, y3), FOREGROUND);
    

	//FILES_RECT
	if((state == STATE_DIR || state == STATE_SPOTIFY) && draw_files) {
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
	const char *text = NULL;
	if(state == STATE_DIR || state == STATE_SPOTIFY) {
	    text = dir_buffer.data + dir_buffer_pos + dir_path_len;
	} else {
	    text = path;
	}
	render_line(&renderer,
		    0, border.y - (float) font.height,
		    &font, text, vec4f(1, 1, 1, 1));

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
	if((state == STATE_DIR || state == STATE_SPOTIFY) && draw_files) {
	    float file_y = draw_files_y + draw_files_height - 1 * bar_margin - (float) font.height;
	    size_t dir_off = 0;
	    size_t payload_off = 0;
	    while(dir_off < dir_buffer.len) {
		float len = font_estimate_width2(&font, dir_buffer.data + dir_path_len + dir_off);
		bool is_available = state == STATE_DIR || dir_off <= (size_t) dir_queried_pos;
		if(is_available && click &&
		   mousex > draw_files_x &&
		   mousey > file_y &&
		   mousex - draw_files_x < len &&
		   mousey - file_y < (float) font.height) {

		    dir_buffer_pos = dir_off;
		    payload_buffer_pos = payload_off;
	  
		    player_close(&player);
		    if(state == STATE_DIR) {
			if(!player_open_file(&player, dir_buffer.data + dir_buffer_pos)) {
			    panic("player_open_file");
			}
		    } else {
			if(!player_open_url(&player, payload_buffer.data + payload_off)) {
			    panic("player_open_url");
			}
		    }

		    if(!player_play(&player)) {
			panic("player_play");
		    }

	  
		}

		Vec4f color = WHITE;
		if(!is_available) {
		    color = GREY_WHITE;
		} else if((size_t) dir_buffer_pos == dir_off) {
		    color = FOREGROUND;
		} 
	
		render_line(&renderer, draw_files_x, file_y, &font, dir_buffer.data + dir_path_len + dir_off, color);
		file_y -= (float) font.height;
		dir_off += strlen(dir_buffer.data + dir_off) + 1;
		if(state == STATE_SPOTIFY) {
		    payload_off += strlen(payload_buffer.data + payload_off) + 1;
		}
	    }

	}

	text = tprintf(&temp, "%.2f", _time);
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
	    if(!player.decoder_used) {
		play(state);
		if(!player_play(&player)) {
		    panic("player_play");
		}
	    } else {
		player_toggle(&player);	
	    }
	}
	renderer_image_rect(&renderer,
			    pos,
			    vec2f(button_width,  button_width),
			    vec2f(player.playing ? .25 : 0, 0), vec2f(.25, 1));

	//PREV
	Vec2f _pos = vec2f_sub(pos, vec2f(2*button_width, 0));
	if((state == STATE_DIR || state == STATE_SPOTIFY) && click &&
	   mousex > _pos.x &&
	   mousey > _pos.y &&
	   mousex - _pos.x < button_width &&
	   mousey - _pos.y < button_width) {

	    dir_buffer_pos -= 2;
	    if(dir_buffer_pos < 0) {
		dir_buffer_pos = dir_buffer.len - 1;
	    }
	    while(dir_buffer_pos > 0 &&
		  dir_buffer.data[dir_buffer_pos - 1] != 0) {
		dir_buffer_pos--;
	    }

	    payload_buffer_pos -= 2;
	    if(payload_buffer_pos < 0) {
		payload_buffer_pos = payload_buffer.len - 1;
	    }
	    while(payload_buffer_pos > 0 &&
		  payload_buffer.data[payload_buffer_pos - 1] != 0) {
		payload_buffer_pos--;
	    }
	  
	    player_close(&player);
      
	    if(state == STATE_DIR) {
		if(!player_open_file(&player, dir_buffer.data + dir_buffer_pos)) {
		    panic("player_open_file");
		}	  
	    } else {
		if(!player_open_url(&player, payload_buffer.data + payload_buffer_pos)) {
		    panic("player_open_url");
		}
	    }
      
	    if(!player_play(&player)) {
		panic("player_play");
	    }
	  
	}	
	renderer_image_rect(&renderer, _pos, vec2f(button_width,  button_width), vec2f(.5, 0), vec2f(.25, 1));

	//NEXT
	_pos = vec2f_add(pos, vec2f(2*button_width, 0));
	if((state == STATE_DIR || state == STATE_SPOTIFY ) && click &&
	   mousex > _pos.x &&
	   mousey > _pos.y &&
	   mousex - _pos.x < button_width &&
	   mousey - _pos.y < button_width) {


	    dir_buffer_pos += strlen(dir_buffer.data + dir_buffer_pos) + 1;
	    if((size_t) dir_buffer_pos >= dir_buffer.len) dir_buffer_pos = 0;

	    if(state == STATE_SPOTIFY) {
		payload_buffer_pos += strlen(payload_buffer.data + payload_buffer_pos) + 1;
		if((size_t) payload_buffer_pos >= payload_buffer.len) payload_buffer_pos = 0;	
	    }

	    if(player.decoder_used) {
		player_close(&player);
	
		if(state == STATE_DIR) {
		    if(!player_open_file(&player, dir_buffer.data + dir_buffer_pos)) {
			panic("player_open_file");
		    }	  
		} else {
		    if(!player_open_url(&player, payload_buffer.data + payload_buffer_pos)) {
			panic("player_open_url");
		    }
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

