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
  String_Buffer sources;
  Arr *sources_offsets;  
  String_Buffer names;
  Arr *names_offsets;  
  size_t pos;
  size_t len;

  Youtube_Context yt_context;
  bool using_yt_context;
}Playlist;

PLAYLIST_DEF void playlist_init(Playlist *playlist);
PLAYLIST_DEF bool playlist_from(Playlist *playlist, Player *player, const char *file_path);
PLAYLIST_DEF bool playlist_from_file(Playlist *playlist, Player *player, const char *file_path);
PLAYLIST_DEF bool playlist_from_dir(Playlist *playlist, Player *player, const char *dir_path);
PLAYLIST_DEF bool playlist_from_youtube(Playlist *playlist, Player *player, const char *link);
PLAYLIST_DEF bool playlist_from_spotify(Playlist *playlist, Player *player, const char *link);
PLAYLIST_DEF void playlist_next(Playlist *playlist);
PLAYLIST_DEF void playlist_prev(Playlist *playlist);
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
  }while(0)

PLAYLIST_DEF void playlist_init(Playlist *playlist) {
  playlist->sources = (String_Buffer) {0};
  playlist->sources_offsets = arr_init(sizeof(size_t));
  playlist->names = (String_Buffer) {0};
  playlist->names_offsets = arr_init(sizeof(size_t));
  playlist->using_yt_context = false;
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
  }

  return false;
}

PLAYLIST_DEF bool playlist_from_file(Playlist *playlist, Player *player, const char *file_path) {
  playlist->sources.len = 0;
  playlist->names.len = 0;
  playlist->pos = 0;
  playlist->len = 0;

  bool is_file;
  if(!io_exists(file_path, &is_file) || !is_file) {
    return false;
  }

  if(!player_open_file(player, file_path)) {
    return false;
  }
  player_close(player);

  PLAYLIST_APPEND(playlist, file_path, file_path); 
  return true;
}

PLAYLIST_DEF bool playlist_from_dir(Playlist *playlist, Player *player, const char *dir_path) {
  playlist->sources.len = 0;
  playlist->names.len = 0;
  playlist->pos = 0;
  playlist->len = 0;
  
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
  playlist->sources.len = 0;
  playlist->names.len = 0;
  playlist->pos = 0;
  playlist->len = 0;

  string videoId;
  if(!youtube_get_videoId(link, &videoId)) {
    return false;
  }

  if(!playlist->using_yt_context) {
    if(!youtube_context_init(&playlist->yt_context)) {
      return false;
    }
    playlist->using_yt_context = true;
  }
  youtube_context_start(&playlist->yt_context);

  char *url, *name = NULL;
  if(!youtube_get_audio(&playlist->yt_context, videoId, &url, &name)) {
    return false;
  }

  if(!player_open_url(player, url)) {
    return false;
  }
  player_close(player);

  PLAYLIST_APPEND(playlist, name, url);
  
  free(name);
  free(url);

  youtube_context_stop(&playlist->yt_context);
  
  return true;
}

PLAYLIST_DEF bool playlist_track_from_keyword(Playlist *playlist, Player *player, const char *keyword) {  
  Json results;
  if(!youtube_search(&playlist->yt_context, keyword, &results)) {
    youtube_context_stop(&playlist->yt_context);
    return false;
  }

  Json result = json_opt(results, 0);
  const char *videoId = json_get_string(result, "videoId");

  char *url, *name = NULL;
  if(!youtube_get_audio(&playlist->yt_context, string_from_cstr(videoId), &url, &name)) {
    json_free_all(results);
    youtube_context_stop(&playlist->yt_context);
    return false;
  }

  if(!player_open_url(player, url)) {
    json_free_all(results);
    youtube_context_stop(&playlist->yt_context);
    return false;
  }
  player_close(player);

  PLAYLIST_APPEND(playlist, name, url);  
  free(name);
  free(url);
  json_free_all(results);

  return true;
}

PLAYLIST_DEF bool playlist_from_spotify(Playlist *playlist, Player *player, const char *_link) {
  playlist->sources.len = 0;
  playlist->names.len = 0;
  playlist->pos = 0;
  playlist->len = 0;

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

    char keyword[1024]; 
    if(!spotify_get_keyword(track, keyword)) {
      return false;
    }
    json_free_all(track);

    result = playlist_track_from_keyword(playlist, player, keyword);    
  } else {

    Json spotify_tracks;
    if(!spotify_by_id2(prefix, link.data, &spotify_tracks)) {
      youtube_context_stop(&playlist->yt_context);
      return false;
    }

    char keyword[1024];
    Json tracks = json_get(json_get(spotify_tracks, "tracks"), "items");
    for(s32 i=0;i<json_size(tracks) && i<1;i++) {
      Json track = json_opt(tracks, i);
      if(json_has(track, "track")) {
	track = json_get(track, "track");
      }

      const char *name = json_get_string(track, "name");
      if(!spotify_get_keyword(track, keyword)) {
	fprintf(stderr, "WARNING: Failed to build keyword for song: '%s'\n", name);
	continue;
      }
    }
    result = playlist_track_from_keyword(playlist, player, keyword);
    json_free_all(spotify_tracks);
  }
    
  youtube_context_stop(&playlist->yt_context);
  return result;
}

PLAYLIST_DEF void playlist_next(Playlist *playlist) {
  playlist->pos = (playlist->pos + 1) % playlist->len;
}

PLAYLIST_DEF void playlist_prev(Playlist *playlist) {
  if(playlist->pos == 0) playlist->pos = playlist->len - 1;
  else playlist->pos--;
}

PLAYLIST_DEF const char *playlist_get_source(Playlist *playlist, size_t pos) {
  if(pos >= playlist->len) return NULL;
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
