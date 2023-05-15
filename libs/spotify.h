#ifndef SPOTIFY_H_H
#define SPOTIFY_H_H

#ifndef SPOTIFY_DEF
#define SPOTIFY_DEF static inline
#endif //SPOTIFY_DEF

#ifdef SPOTIFY_IMPLEMENTATION

#define HTTP_IMPLEMENTATION
#define JSON_IMPLEMENTATION
#define BASE64_IMPLEMENTATION

#endif //SPOTIFY_IMPLEMENTATION

#include "http.h"
#include "json.h"
#include "base64.h"

SPOTIFY_DEF bool spotify_bearer_token(char *output_buffer, u64 output_size);
SPOTIFY_DEF bool spotify_by_id(const char *prefix, const char *id, Json *json);
SPOTIFY_DEF bool spotify_by_id2(string prefix, const char *id, Json *json);
SPOTIFY_DEF bool spotify_playlist_by_id(const char* playlistId, Json *json);
SPOTIFY_DEF bool spotify_album_by_id(const char* albumId, Json *json);
SPOTIFY_DEF bool spotify_track_by_id(const char* track, Json *json);
SPOTIFY_DEF bool spotify_get_keyword(const Json track, char keyword[1024]);

#ifdef SPOTIFY_IMPLEMENTATION

SPOTIFY_DEF bool spotify_bearer_token(char *output_buffer, u64 output_size) {

  char spotify_creds[1024];
  if(!io_getenv("SPOTIFY_CREDS", spotify_creds, sizeof(spotify_creds))) {
    fprintf(stderr, "ERROR: The envorinment variable 'SPOTIFY_CREDS' is not set.\n");
    fprintf(stderr, "It should be set this way: 'clientId:clientSecrect'\n");
    return false;    
  }

#define BUFFER_SIZE 1024
  char buffer[2][BUFFER_SIZE];
  size_t buffer_size = strlen(spotify_creds);
  
  size_t out_buffer_size;
  if(!base64_encode(spotify_creds, buffer_size,
		    buffer[1], BUFFER_SIZE - 1,
		    &out_buffer_size)) {
    return false;
  }
  buffer[1][out_buffer_size] = 0;
  
  if((buffer_size = snprintf(buffer[0], BUFFER_SIZE, "Authorization: Basic %s\r\n", buffer[1])) >= BUFFER_SIZE) {
    return false;
  }

  String_Buffer sb = {0};
  if(!http_post("https://accounts.spotify.com/api/token",
		"grant_type=client_credentials",
		"application/x-www-form-urlencoded",
		string_buffer_callback, &sb, NULL, buffer[0])) {
    return false;
  }

  Json json;
  if(!json_parse_len(sb.data, sb.len, &json)) {
    string_buffer_free(&sb);
    return false;
  }

  const char* bearerToken = json_get_string(json, "access_token");
  size_t len = strlen(bearerToken);
  if(len < output_size - 1) {
    memcpy(output_buffer, bearerToken, len + 1);
  }

  string_buffer_free(&sb);
  json_free_all(json);

  return len < (output_size - 1);
}

SPOTIFY_DEF bool spotify_by_id2(string prefix, const char *id, Json *json) {
  char bearer_token[1024];
  if(!spotify_bearer_token(bearer_token, 1024)) {
    return false;
  }

  char buffer[2][1024 * 10];
  if(snprintf(buffer[0], 1024 * 10, "Authorization: Bearer %s\r\n", bearer_token) >= (1024 * 10)) {
    return false;
  }

  if(snprintf(buffer[1], 1024 * 10, "https://api.spotify.com/v1/"String_Fmt"s/%s", String_Arg(prefix), id) >= (1024 * 10)) {
    return false;
  }
  
  String_Buffer sb = {0};
  if(!http_get(buffer[1], string_buffer_callback, &sb, NULL, buffer[0])) {
    return false;
  }

  if(!json_parse_len(sb.data, sb.len, json)) {
    return false;
  }
  
  string_buffer_free(&sb);
  return true;    
}

SPOTIFY_DEF bool spotify_by_id(const char *prefix, const char *id, Json *json) {
  return spotify_by_id2(string_from_cstr(prefix), id, json);
}

SPOTIFY_DEF bool spotify_track_by_id(const char *trackId, Json *json) {
  return spotify_by_id("track", trackId, json);
}

SPOTIFY_DEF bool spotify_playlist_by_id(const char* playlistId, Json *json) {
  return spotify_by_id("playlist", playlistId, json);
}

SPOTIFY_DEF bool spotify_album_by_id(const char* albumId, Json *json) {
  return spotify_by_id("album", albumId, json);
}

SPOTIFY_DEF bool spotify_get_keyword(const Json track, char keyword[1024]) {
  char temp[1024];
  
  if(!json_has(track, "name")) {
    return false;
  }
  const char *name = json_get_string(track, "name");
  size_t name_len = strlen(name);
  
  size_t pos = 0;
  if(name_len >= 1024) {
    return false;
  }
  memcpy(temp, name, name_len);
  pos += name_len;

  Json artists = json_get(track, "artists");
  for(s32 j=0;j<json_size(artists);j++) {
    Json artist = json_opt(artists, j);
    const char* artist_name = json_get_string(artist, "name");
    size_t artist_name_len = strlen(artist_name);
    if(pos + artist_name_len + 1 >= 1024) {
      return false;
    }
    temp[pos++] = ' ';
    memcpy(temp + pos, artist_name, artist_name_len);      
    pos += artist_name_len;
  }

  size_t keyword_len;
  if(!http_encodeURI(temp, pos, keyword, 1023, &keyword_len)) {
    return false;
  }
  keyword[keyword_len] = 0;

  return true;
}

#endif //SPOTIFY_IMPLEMENTATION

#endif //SPOTIFY_H_H
