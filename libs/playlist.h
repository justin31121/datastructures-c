#ifndef PLAYLIST_H_H_
#define PLAYLIST_H_H_

#ifdef PLAYLIST_IMPLEMENTATION

#define IO_IMPLEMENTATION
#define ARRAY_IMPLEMENTATION
#define PLAYER_IMPLEMENTATION
#define YOUTUBE_IMPLEMENTATION
#define SPOTIFY_IMPLEMENTATION

#endif //PLAYLIST_IMPLEMENTATION

#ifndef PLAYLIST_DEF
#define PLAYLIST_DEF static inline
#endif //PLAYLIST_DEF

#include "./array.h"
#include "./io.h"
#include "./player.h"
#include "./youtube.h"
#include "./spotify.h"

typedef struct{
  Http http;
  String_Buffer sb;
  duk_context *duk_ctx;
  Spotify_Tracks tracks;
}Playlist_From_Spotify_Context;

typedef struct{
  String_Buffer sources;
  Arr *sources_offsets;  
  String_Buffer names;
  Arr *names_offsets;  
  size_t pos;
  size_t len;
  size_t available;

  Youtube_Context yt_context;
  bool using_yt_context;

  Playlist_From_Spotify_Context context;
  Json spotify_tracks;
}Playlist;

PLAYLIST_DEF void playlist_init(Playlist *playlist);
PLAYLIST_DEF bool playlist_from(Playlist *playlist, Player *player, const char *file_path);
PLAYLIST_DEF bool playlist_from_file(Playlist *playlist, Player *player, const char *file_path);
PLAYLIST_DEF bool playlist_from_url(Playlist *playlist, Player *player, const char *file_path);
PLAYLIST_DEF bool playlist_from_dir(Playlist *playlist, Player *player, const char *dir_path);
PLAYLIST_DEF bool playlist_from_youtube(Playlist *playlist, Player *player, const char *link);

PLAYLIST_DEF bool playlist_from_spotify(Playlist *playlist, Player *player, const char *link);
PLAYLIST_DEF bool playlist_track_from_keyword(Playlist *playlist, Player *player, const char *keyword);
PLAYLIST_DEF void *playlist_tracks_from_spotify_thread(void *arg);

PLAYLIST_DEF void playlist_next(Playlist *playlist);
PLAYLIST_DEF void playlist_prev(Playlist *playlist);
PLAYLIST_DEF bool playlist_is_enabled(Playlist *playlist, size_t pos);
PLAYLIST_DEF const char *playlist_get_name(Playlist *playlist, size_t pos);
PLAYLIST_DEF const char *playlist_get_source(Playlist *playlist, size_t pos);
PLAYLIST_DEF void playlist_free(Playlist *playlist);

#ifdef PLAYLIST_IMPLEMENTATION

#define PLAYLIST_APPEND(playlist, name, source) do{			\
    arr_push((playlist)->sources_offsets, &(playlist)->sources.len);	\
    string_buffer_append(&(playlist)->sources, (source), strlen((source))+1); \
    arr_push((playlist)->names_offsets, &(playlist)->names.len);	\
    string_buffer_append(&(playlist)->names, (name), strlen(name)+1);	\
    (playlist)->len++;							\
    (playlist)->available++;						\
  }while(0)

#define PLAYLIST_NAME_APPEND(playlist, name) do{			\
    arr_push((playlist)->names_offsets, &(playlist)->names.len);	\
    string_buffer_append(&(playlist)->names, (name), strlen(name)+1);	\
    (playlist)->len++;							\
  }while(0)

#define PLAYLIST_NAME_LEN_APPEND(playlist, name, name_len) do{		\
    arr_push((playlist)->names_offsets, &(playlist)->names.len);	\
    string_buffer_append(&(playlist)->names, (name), (name_len));	\
    string_buffer_append(&(playlist)->names, "\0", 1);			\
    (playlist)->len++;							\
  }while(0)

#define PLAYLIST_SOURCE_APPEND(playlist, source) do{			\
    arr_push((playlist)->sources_offsets, &(playlist)->sources.len);	\
    string_buffer_append(&(playlist)->sources, (source), strlen((source))+1); \
    (playlist)->available++;						\
  }while(0)

#define PLAYLIST_SOURCE_LEN_APPEND(playlist, source, source_len) do{	\
    arr_push((playlist)->sources_offsets, &(playlist)->sources.len);	\
    string_buffer_append(&(playlist)->sources, (source), (source_len)); \
    string_buffer_append(&(playlist)->sources, "\0", 1);		\
    (playlist)->available++;						\
  }while(0)

PLAYLIST_DEF void playlist_init(Playlist *playlist) {
  playlist->sources = (String_Buffer) {0};
  playlist->sources_offsets = arr_init(sizeof(size_t));
  playlist->names = (String_Buffer) {0};
  playlist->names_offsets = arr_init(sizeof(size_t));
  playlist->using_yt_context = false;
  playlist->pos = 0;
  playlist->len = 0;
  playlist->available = 0;

  string_buffer_reserve(&playlist->sources, 1024);
  string_buffer_reserve(&playlist->names, 1024);
}

PLAYLIST_DEF bool playlist_from(Playlist *playlist, Player *player, const char *arg) {
  if(playlist_from_file(playlist, player, arg)) {
    return true;
  } else if(playlist_from_dir(playlist, player, arg)) {
    return true;
  } else if(playlist_from_youtube(playlist, player, arg)) {
    return true;
  } else if(playlist_from_spotify(playlist, player, arg)) {
    return true;
  } else if(playlist_from_url(playlist, player, arg)) {
    return true;
  }
  return false;
}

PLAYLIST_DEF bool playlist_from_file(Playlist *playlist, Player *player, const char *file_path) {
  playlist->sources.len = 0;
  playlist->names.len = 0;

  bool is_file;
  if(!io_exists(file_path, &is_file) || !is_file) {
    return false;
  }

  if(!player_open_file(player, file_path)) {
    return false;
  }
  player_close(player);

#ifdef _WIN32
  char delim = '\\';
#elif linux
  char delim = '/';
#endif

  string filepath = string_from_cstr(file_path);
  string last = {0};
  while(filepath.len) {
    string part = string_chop_by_delim(&filepath, delim);
    last = part;
  }

  PLAYLIST_NAME_LEN_APPEND(playlist, last.data, last.len);
  PLAYLIST_SOURCE_APPEND(playlist, file_path);
  
  return true;
}

PLAYLIST_DEF bool playlist_from_url(Playlist *playlist, Player *player, const char *path) {
  playlist->sources.len = 0;
  playlist->names.len = 0;

  if(!player_open_url(player, path)) {
    return false;
  }
  player_close(player);

  PLAYLIST_APPEND(playlist, "url", path);
  return true;
}

PLAYLIST_DEF bool playlist_from_dir(Playlist *playlist, Player *player, const char *dir_path) {
  playlist->sources.len = 0;
  playlist->names.len = 0;
  
  size_t dir_path_len = strlen(dir_path);

  Io_Dir dir;
  if(!io_dir_open(&dir, dir_path)) {
    return false;
  }

  Io_File file;
  while(io_dir_next(&dir, &file)) {
    if(file.is_dir) continue;
    if(!player_open_file(player, file.abs_name)) {
      continue;
    }

    const char *name = file.abs_name + dir_path_len;
    PLAYLIST_APPEND(playlist, name, file.abs_name);
    
    player_close(player);
  }

  io_dir_close(&dir);
  
  return true;
}

PLAYLIST_DEF bool playlist_from_youtube(Playlist *playlist, Player *player, const char *link) {

  string videoId;
  if(!youtube_get_videoId(link, &videoId)) {
    return false;
  }

  Http http;
  if(!http_init2(&http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
    panic("http_init2");
  }
  String_Buffer temp = {0};
  string_buffer_reserve(&temp, 1024 * 1024 * 5);
  duk_context* duk_ctx = duk_create_heap_default();

  string _url;
  if(!youtube_get_audio2(videoId, &http, &temp, duk_ctx, &_url, NULL)) {
    panic("youtube_get_audio2");
  }

  PLAYLIST_NAME_APPEND(playlist, "youtube video ...");
  PLAYLIST_SOURCE_LEN_APPEND(playlist, _url.data, _url.len);

  if(!player_open_url(player, playlist_get_source(playlist, 0) )) {
    return false;
  }
  player_close(player);

  duk_destroy_heap(duk_ctx);
  string_buffer_free(&temp);
  http_free(&http);
  
  return true;
}

PLAYLIST_DEF void *playlist_tracks_from_spotify_thread(void *arg) {
  Playlist *playlist = (Playlist *) arg;

  char keyword[1024];
  Json tracks = json_get(json_get(playlist->spotify_tracks, "tracks"), "items");
  for(s32 i=1;i<json_size(tracks) && i<30;i++) {
    Json track = json_opt(tracks, i);
    if(json_has(track, "track")) {
      track = json_get(track, "track");
    }

    if(!spotify_get_keyword(track, keyword)) {
      break;
    }
    
    playlist_track_from_keyword(playlist, NULL, keyword);
  }
 
  json_free_all(playlist->spotify_tracks);
  youtube_context_stop(&playlist->yt_context);
  return NULL;
}

PLAYLIST_DEF bool playlist_track_from_keyword(Playlist *playlist, Player *player, const char *keyword) {  
  Json results;
  if(!youtube_search(&playlist->yt_context, keyword, &results)) {
    return false;
  }

  Json result = json_opt(results, 0);
  const char *videoId = json_get_string(result, "videoId");

  char *url;
  if(!youtube_get_audio(&playlist->yt_context, string_from_cstr(videoId), &url, NULL)) {
    json_free_all(results);
    return false;
  }

  (void) player;
  //TODO: maybe add a second player for validation or dont
  
  /*
    if(!player_open_url(player, url)) {
    json_free_all(results);
    return false;
    }
    player_close(player);
  */

  PLAYLIST_SOURCE_APPEND(playlist, url);
  free(url);
  json_free_all(results);

  return true;
}

PLAYLIST_DEF void *playlist_from_spotify_thread(void *arg) {
  Playlist *playlist = (Playlist *) arg;
  Playlist_From_Spotify_Context* context = &playlist->context;
  (void) context;

  size_t sb_len = context->sb.len;

  bool first = true;
  char name_buf[1024];
  size_t name_size, name_off;
  while(spotify_tracks_next(&context->tracks, name_buf, sizeof(name_buf), &name_size, &name_off) ) {
    context->sb.len = sb_len;
    if(first) {
      first = false;
      continue;
    }
    string name = string_from(name_buf, name_size);
    //string short_name = string_from(name_buf + name_off, name_size - name_off);
    //printf( String_Fmt" ("String_Fmt")\n", String_Arg(name), String_Arg(short_name) );
    //PLAYLIST_NAME_LEN_APPEND(playlist, short_name.data, short_name.len);

    string videoId;
    if(!youtube_results_first(name, &context->http, &context->sb, &videoId)) {
      panic("youtube_results_first");
    }
    //printf(String_Fmt"\n", String_Arg(videoId));

    string _url;
    if(!youtube_get_audio2(videoId, &context->http, &context->sb, context->duk_ctx, &_url, NULL)) {
      panic("youtube_get_audio2");
    }
	
    PLAYLIST_SOURCE_LEN_APPEND(playlist, _url.data, _url.len);
  }

  spotify_track_names_free(&context->tracks);
  duk_destroy_heap(context->duk_ctx);
  string_buffer_free(&context->sb);
  http_free(&context->http);
  return NULL;
}

PLAYLIST_DEF bool playlist_from_spotify(Playlist *playlist, Player *player, const char *_link) {
  (void) player;
  string_buffer_reserve(&playlist->sources, 1024 * 4);

  string link = string_from_cstr(_link);
  if(!string_chop_string(&link, STRING("https://open.spotify.com/"))) {
    return false;
  }
  string_chop_string(&link, STRING("intl-de/"));

  string prefix = {0};
  bool is_track = string_chop_string(&link, STRING("track/"));
  if(!is_track) {
    prefix = string_chop_by_delim(&link, '/');
  }

  char spotify_creds[1024];
  if(!io_getenv("SPOTIFY_CREDS", spotify_creds, sizeof(spotify_creds))) {
    fprintf(stderr, "ERROR: The envorinment variable 'SPOTIFY_CREDS' is not set.\n");
    fprintf(stderr, "It should be set this way: 'clientId:clientSecrect'\n");
    return false;
  }

  playlist->context = (Playlist_From_Spotify_Context) {0};
  if(!http_init2(&playlist->context.http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
    panic("http_init2");
  }
  playlist->context.duk_ctx = duk_create_heap_default();
  string_buffer_reserve(&playlist->context.sb, 1024 * 1024 * 16);
    
  string access_token;
  if(!spotify_get_access_token(spotify_creds, &playlist->context.sb, &access_token)) {
    panic("get_access_token");
  }

  if(is_track) {
    string name;
    if(!spotify_get_track_name(access_token, link, &playlist->context.sb, &name)) {
      panic("get_track_name");
    }
    PLAYLIST_NAME_LEN_APPEND(playlist, name.data, name.len);

    string videoId;
    if(!youtube_results_first(name, &playlist->context.http, &playlist->context.sb, &videoId)) {
      panic("youtube_results_first");
    }

    string _url;
    if(!youtube_get_audio2(videoId, &playlist->context.http, &playlist->context.sb, playlist->context.duk_ctx, &_url, NULL)) {
      panic("youtube_get_audio2");
    }
	
    PLAYLIST_SOURCE_LEN_APPEND(playlist, _url.data, _url.len);
	
    duk_destroy_heap(playlist->context.duk_ctx);
    string_buffer_free(&playlist->context.sb);
    http_free(&playlist->context.http);

    return true;
  } else {
    if(!spotify_get_track_names(access_token, prefix, link, &playlist->context.sb, &playlist->context.tracks)) {
      panic("get_track_names");
    }
	
    size_t tracks_pos = playlist->context.tracks.pos;

    char name_buf[1024];
    size_t name_size, name_off;
    if(!spotify_tracks_next(&playlist->context.tracks, name_buf, sizeof(name_buf), &name_size, &name_off) ) {
      return false;
    }
    string name = string_from(name_buf, name_size);
    string short_name = string_from(name_buf + name_off, name_size - name_off);
	
    PLAYLIST_NAME_LEN_APPEND(playlist, short_name.data, short_name.len);

    string videoId;
    if(!youtube_results_first(name, &playlist->context.http, &playlist->context.sb, &videoId)) {
      panic("youtube_results_first");
    }

    string _url;
    if(!youtube_get_audio2(videoId, &playlist->context.http, &playlist->context.sb, playlist->context.duk_ctx, &_url, NULL)) {
      panic("youtube_get_audio2");
    }
	
    PLAYLIST_SOURCE_LEN_APPEND(playlist, _url.data, _url.len);

    bool first = true;
    playlist->context.tracks.pos = tracks_pos;
    while(spotify_tracks_next(&playlist->context.tracks, name_buf, sizeof(name_buf), &name_size, &name_off) ) {
      if(first) {
	first = false;
	continue;
      }
      short_name = string_from(name_buf + name_off, name_size - name_off);
      PLAYLIST_NAME_LEN_APPEND(playlist, short_name.data, short_name.len);
    }

    playlist->context.tracks.pos = tracks_pos;
    Thread spotify_thread;
    if(!thread_create(&spotify_thread, playlist_from_spotify_thread, playlist)) {
      return false;
    }

    return true;
  }
}

PLAYLIST_DEF bool playlist_from_spotify2(Playlist *playlist, Player *player, const char *_link) {
  string_buffer_reserve(&playlist->sources, 1024 * 4);
  playlist->sources.len = 0;
  playlist->names.len = 0;

  string link = string_from_cstr(_link);
  if(!string_chop_string(&link, STRING("https://open.spotify.com/"))) {
    return false;
  }
  string_chop_string(&link, STRING("intl-de/"));

  string prefix = {0};
  bool is_track = string_chop_string(&link, STRING("track/"));
  if(!is_track) {
    prefix = string_chop_by_delim(&link, '/');
  }
  
  if(!playlist->using_yt_context) {
    if(!youtube_context_init(&playlist->yt_context)) {
      return false;
    }
    playlist->using_yt_context = true;
  }
  youtube_context_start(&playlist->yt_context);

  bool result = true;
  if(is_track) {
    Json track;
    if(!spotify_track_by_id(link.data, &track)) {
      return false;
    }

    const char *name = json_get_string(track, "name");

    char keyword[1024]; 
    if(!spotify_get_keyword(track, keyword)) {
      return false;
    }
    json_free_all(track);

    PLAYLIST_NAME_APPEND(playlist, name);
    result = playlist_track_from_keyword(playlist, player, keyword);
    youtube_context_stop(&playlist->yt_context);
  } else {

    if(!spotify_by_id2(prefix, link.data, &playlist->spotify_tracks)) {
      youtube_context_stop(&playlist->yt_context);
      return false;
    }

    char keyword[1024];
    Json tracks = json_get(json_get(playlist->spotify_tracks, "tracks"), "items");
    Json track;
    for(s32 i=0;i<json_size(tracks) && i<30;i++) {
      track = json_opt(tracks, i);
      if(json_has(track, "track")) {
	track = json_get(track, "track");
      }
      
      const char *name = json_get_string(track, "name");      
      PLAYLIST_NAME_APPEND(playlist, name);
    }

    track = json_opt(tracks, 0);
    if(json_has(track, "track")) {
      track = json_get(track, "track");
    }
    if(!spotify_get_keyword(track, keyword)) {
      result = false;
    }
    result = playlist_track_from_keyword(playlist, player, keyword);

    Thread spotify_thread;
    if(!thread_create(&spotify_thread, playlist_tracks_from_spotify_thread, playlist)) {
      return false;
    }  
  }

  return result;
}

PLAYLIST_DEF void playlist_next(Playlist *playlist) {
  playlist->pos = (playlist->pos + 1) % playlist->available;
}

PLAYLIST_DEF void playlist_prev(Playlist *playlist) {
  if(playlist->pos == 0) playlist->pos = playlist->available - 1;
  else playlist->pos--;
}

PLAYLIST_DEF bool playlist_is_enabled(Playlist *playlist, size_t pos) {
  return pos < playlist->available;
}

PLAYLIST_DEF const char *playlist_get_source(Playlist *playlist, size_t pos) {
  if(pos >= playlist->available) return NULL;
  size_t off = *(size_t *) arr_get(playlist->sources_offsets, pos);
  return playlist->sources.data + off;
}

PLAYLIST_DEF const char *playlist_get_name(Playlist *playlist, size_t pos) {
  if(pos >= playlist->len) return NULL;
  size_t off = *(size_t *) arr_get(playlist->names_offsets, pos);
  return playlist->names.data + off;
}

PLAYLIST_DEF void playlist_free(Playlist *playlist) {
  string_buffer_free(&playlist->sources);
  string_buffer_free(&playlist->names);
  arr_free(playlist->sources_offsets);
  arr_free(playlist->names_offsets);
  if(playlist->using_yt_context) {
    youtube_context_free(&playlist->yt_context);
  }
}

#endif //PLAYLIST_IMPLEMENTATION

#endif //PLAYLIST_H_H_
