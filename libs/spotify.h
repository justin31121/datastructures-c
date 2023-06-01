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

typedef struct{
    Arr *names;
    string prev;
    size_t count;
    size_t pos;
    bool is_playlist;    
}Spotify_Tracks;

SPOTIFY_DEF bool spotify_get_access_token(char *spotify_creds, String_Buffer *sb, string *access_token);
SPOTIFY_DEF bool spotify_get_track_name(string access_token, string id, String_Buffer *sb, string *name);

SPOTIFY_DEF bool spotify_get_track_names(string access_token, string prefix, string id, String_Buffer *temp, Spotify_Tracks *tracks);
SPOTIFY_DEF string spotify_tracks_get_folder(Spotify_Tracks *tracks);
SPOTIFY_DEF bool spotify_tracks_next(Spotify_Tracks *tracks, char *buffer, size_t buffer_cap, size_t *buffer_size, size_t *buffer_off);
SPOTIFY_DEF void spotify_track_names_free(Spotify_Tracks *tracks);

SPOTIFY_DEF bool spotify_get_track_names2(string access_tkoen, string prefix, string id, String_Buffer *sb, Arr *names);

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

/////////////////////////////////////////////////////////////////////////////////////

typedef struct{
    string prev;
    string token;
}Spotify_Auth;

SPOTIFY_DEF bool spotify_on_elem_access_token(Json_Parse_Type type, string content, void *arg, void **elem) {
    (void) type;
    (void) elem;
  
    Spotify_Auth *auth = (Spotify_Auth *) arg;
    auth->prev = content;
    return true;
}

SPOTIFY_DEF void spotify_on_object_elem_access_token(void *object, string key, void *elem, void *arg) {
    (void) object;
    (void) elem;
  
    Spotify_Auth *auth = (Spotify_Auth *) arg;  
    if(string_eq_cstr(key, "access_token")) {
	auth->token = auth->prev;
    }
}

SPOTIFY_DEF bool spotify_get_access_token(char *spotify_creds, String_Buffer *sb, string *access_token) {
    size_t sb_len = sb->len;
    string key = tsmap(sb, string_from_cstr(spotify_creds), base64_encode);
    const char *auth = tprintf(sb, "Authorization: Basic "String_Fmt"\r\n", String_Arg( key ));

    sb->len = sb_len;
    if(!http_post("https://accounts.spotify.com/api/token",
		  "grant_type=client_credentials",
		  "application/x-www-form-urlencoded",
		  string_buffer_callback, sb, NULL, auth)) {
	return false;
    }
    string response = string_from(sb->data + sb_len, sb->len - sb_len);

    Spotify_Auth spotify_auth = {0};
    Json_Parse_Events events = {0};
    events.on_elem = spotify_on_elem_access_token;
    events.on_object_elem = spotify_on_object_elem_access_token;
    events.arg = &spotify_auth;
    if(!json_parse2(response.data, response.len, &events)) {
	return false;
    }
  
    if(!spotify_auth.token.len) {
	return false;
    }
    sb->len = sb_len;
    string_buffer_append(sb, spotify_auth.token.data, spotify_auth.token.len);

    *access_token = string_from(sb->data + sb->len - spotify_auth.token.len, spotify_auth.token.len);
  
    return true;
    
}

typedef struct{
    string prev;
    bool in_artist;
    string name;
    Arr *artists;
}Spotify_Name;

SPOTIFY_DEF bool spotify_on_elem_name(Json_Parse_Type type, string content, void *arg, void **elem) {
    (void) type;
    (void) elem;
  
    Spotify_Name *name = (Spotify_Name *) arg;
    name->prev = content;
    
    return true;
}

SPOTIFY_DEF void spotify_on_object_elem_name(void *object, string key, void *elem, void *arg) {
    (void) object;
    (void) elem;

    Spotify_Name *name = (Spotify_Name *) arg;
    if(string_eq_cstr(key, "artists")) {
	name->in_artist = true;
    }
    if(string_eq_cstr(key, "is_local")) {
	name->in_artist = false;
    }

    if(name->in_artist && string_eq_cstr(key, "name")) {
	arr_push(name->artists, &name->prev);
    }

    if(string_eq_cstr(key, "name")) {
	name->name = name->prev;
    }
}


SPOTIFY_DEF bool spotify_get_track_name(string access_token, string id, String_Buffer *sb, string *name) {
    size_t sb_len = sb->len;
    const char *url = tprintf(sb, "https://api.spotify.com/v1/tracks/"String_Fmt, String_Arg(id));
    const char *auth = tprintf(sb, "Authorization: Bearer "String_Fmt"\r\n", String_Arg(access_token));

    sb->len = sb_len;
    if(!http_get(url, string_buffer_callback, sb, NULL, auth)) {
	return false;
    }
    string response = string_from(sb->data + sb_len, sb->len - sb_len);

    Spotify_Name spotify_name = {0};
    spotify_name.artists = arr_init(sizeof(string)); //TODO: this leaks memory
  
    Json_Parse_Events events = {0};
    events.on_elem = spotify_on_elem_name;
    events.on_object_elem = spotify_on_object_elem_name;
    events.arg = &spotify_name;
    if(!json_parse2(response.data, response.len, &events)) {
	return false;
    }

    char *data = sb->data + sb->len;
    size_t size = spotify_name.name.len;
    string_buffer_append(sb, spotify_name.name.data, spotify_name.name.len);

    for(size_t i=1;i<spotify_name.artists->count;i++) {
	size += 1;
	string_buffer_append(sb, " ", 1);
    
	string artist = *(string *) arr_get(spotify_name.artists, i);
	string_buffer_append(sb, artist.data, artist.len);
	size += artist.len;
    }

    string keyword = string_from(data, size);
  
    sb->len = sb_len;
    string_buffer_append(sb, keyword.data, keyword.len);
  
    *name = string_from(sb->data + sb->len - keyword.len, keyword.len);
  

    return true;

}

string SPOTIFY_TRACKS_FRAME = STRING_STATIC("==SPTFRAME==");

SPOTIFY_DEF bool spotify_on_elem_tracks(Json_Parse_Type type, string content, void *arg, void **elem) {
    (void) type;
    (void) elem;

    Spotify_Tracks *tracks = (Spotify_Tracks *) arg;
    tracks->prev = content;
  
    return true;
}

SPOTIFY_DEF void spotify_on_object_elem_tracks(void *object, string key, void *elem, void *arg) {
    (void) object;
    (void) elem;
    (void) key;

    Spotify_Tracks *tracks = (Spotify_Tracks *) arg;
    if( string_eq_cstr(key, "name") ) {
	arr_push(tracks->names, &tracks->prev);
    }

    if( tracks->names->count > 0 && (string_eq_cstr(key, "track_number") ||
				     string_eq_cstr(key, "total_tracks")) ) {
	arr_push(tracks->names, &SPOTIFY_TRACKS_FRAME);
	tracks->count++;
    }
}

SPOTIFY_DEF bool spotify_get_track_names2(string access_token, string prefix, string id, String_Buffer *sb, Arr *names) {
    size_t sb_len = sb->len;
    const char *url = tprintf(sb, "https://api.spotify.com/v1/"String_Fmt"s/"String_Fmt, String_Arg(prefix), String_Arg(id));
    const char *auth = tprintf(sb, "Authorization: Bearer "String_Fmt"\r\n", String_Arg(access_token));

    sb->len = sb_len;
    if(!http_get(url, string_buffer_callback, sb, NULL, auth)) {
	return false;
    }
    string response = string_from(sb->data + sb_len, sb->len - sb_len);

    Spotify_Tracks tracks = {0};
    tracks.names = names;
  
    Json_Parse_Events events = {0};
    events.on_elem = spotify_on_elem_tracks;
    events.on_object_elem = spotify_on_object_elem_tracks;
    events.arg = &tracks;
    if(!json_parse2(response.data, response.len, &events)) {
	return false;
    }
  
    return true;    
}

SPOTIFY_DEF bool spotify_get_track_names(string access_token, string prefix, string id, String_Buffer *sb, Spotify_Tracks *tracks) {
    size_t sb_len = sb->len;
    const char *url = tprintf(sb, "https://api.spotify.com/v1/"String_Fmt"s/"String_Fmt, String_Arg(prefix), String_Arg(id));
    const char *auth = tprintf(sb, "Authorization: Bearer "String_Fmt"\r\n", String_Arg(access_token));

    sb->len = sb_len;
    if(!http_get(url, string_buffer_callback, sb, NULL, auth)) {
	return false;
    }
    string response = string_from(sb->data + sb_len, sb->len - sb_len);

    *tracks = (Spotify_Tracks) {0};
    tracks->names = arr_init2(sizeof(string), 128);
    tracks->is_playlist = string_eq_cstr(prefix, "playlist");
    if(tracks->is_playlist) {
	tracks->pos = 2;
    }
  
    Json_Parse_Events events = {0};
    events.on_elem = spotify_on_elem_tracks;
    events.on_object_elem = spotify_on_object_elem_tracks;
    events.arg = tracks;
    if(!json_parse2(response.data, response.len, &events)) {
	return false;
    }
    
    string next = *(string *) arr_get(tracks->names, tracks->pos++);
    while(!tracks->is_playlist &&  !string_eq(next, SPOTIFY_TRACKS_FRAME) ) {
	 next = *(string *) arr_get(tracks->names, tracks->pos++);
    }
  
    return true;    
}

SPOTIFY_DEF string spotify_tracks_get_folder(Spotify_Tracks *tracks) {
    return *(string *) arr_get(tracks->names, !tracks->is_playlist);
}

SPOTIFY_DEF bool spotify_tracks_next(Spotify_Tracks *tracks, char *buffer, size_t buffer_cap, size_t *buffer_size, size_t *buffer_off) {
    if(tracks->pos >= tracks->names->count) return false;
    
    string next = *(string *) arr_get(tracks->names, tracks->pos++);	
    while(tracks->is_playlist && !string_eq(next, SPOTIFY_TRACKS_FRAME) ) {
	next = *(string *) arr_get(tracks->names, tracks->pos++);	
    }
    if(tracks->is_playlist) {
	next = *(string *) arr_get(tracks->names, tracks->pos++);
    }

    size_t keyword_size = 0;
    size_t keyword_off = 0;
    while( !string_eq(next, SPOTIFY_TRACKS_FRAME) ) {
	keyword_off = keyword_size + 1;

	if(keyword_size && keyword_size+1 < buffer_cap) buffer[keyword_size++] = ' ';
	if(keyword_size + next.len < buffer_cap) {
	    memcpy(buffer + keyword_size, next.data, next.len );
	    keyword_size += next.len;	    
	}
	
	
	next = *(string *) arr_get(tracks->names, tracks->pos++);
    }

    *buffer_off = keyword_off;
    *buffer_size = keyword_size;
    return true;
}

SPOTIFY_DEF void spotify_track_names_free(Spotify_Tracks *tracks) {
    arr_free(tracks->names);
}

#endif //SPOTIFY_IMPLEMENTATION

#endif //SPOTIFY_H_H
