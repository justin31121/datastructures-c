#define HTTP_IMPLEMENTATION
#include "../libs/http.h"

#define IO_IMPLEMENTATION
#include "../libs/io.h"

#define BASE64_IMPLEMENTATION
#include "../libs/base64.h"

#define JSON_PARSER_IMPLEMENTATION
#include "../libs/json_parser.h"

#define ARRAY_IMPLEMENTATION
#include "../libs/array.h"

typedef struct{
  string prev;
  bool in_artist;
  string name;
  Arr *artists;
}Spotify_Name;

bool on_elem_name(Json_Parse_Type type, string content, void *arg, void **elem) {
  (void) type;
  (void) elem;
  
  Spotify_Name *name = (Spotify_Name *) arg;
  name->prev = content;
    
  return true;
}

void on_object_elem_name(void *object, string key, void *elem, void *arg) {
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

bool get_track_name(string access_token, string id, String_Buffer *sb, string *name) {

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
  events.on_elem = on_elem_name;
  events.on_object_elem = on_object_elem_name;
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

typedef struct{
  string prev;
  string token;
}Spotify_Auth;

bool on_elem(Json_Parse_Type type, string content, void *arg, void **elem) {
  (void) type;
  (void) elem;
  
  Spotify_Auth *auth = (Spotify_Auth *) arg;
  auth->prev = content;
  return true;
}

void on_object_elem(void *object, string key, void *elem, void *arg) {
  (void) object;
  (void) elem;
  
  Spotify_Auth *auth = (Spotify_Auth *) arg;  
  if(string_eq_cstr(key, "access_token")) {
    auth->token = auth->prev;
  }
}

bool get_access_token(char *spotify_creds, String_Buffer *sb, string *access_token) {

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
  events.on_elem = on_elem;
  events.on_object_elem = on_object_elem;
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

String_Buffer temp = {0};

int main(int argc, char **argv) {

  if(argc == 1) {
    fprintf(stderr, "ERROR: Please provide enough arguments\n");
    fprintf(stderr, "USAGE: %s <spotfiy-link>\n", argv[0]);
    return 1;
  }
  const char *_link = argv[1];

  char spotify_creds[1024];
  if(!io_getenv("SPOTIFY_CREDS", spotify_creds, sizeof(spotify_creds))) {
    fprintf(stderr, "ERROR: The envorinment variable 'SPOTIFY_CREDS' is not set.\n");
    fprintf(stderr, "It should be set this way: 'clientId:clientSecrect'\n");
    return 1;
  }

  string_buffer_reserve(&temp, 1024 * 4);

  string access_token;
  if(!get_access_token(spotify_creds, &temp, &access_token)) {
    panic("get_access_token");
  }

  string link = string_from_cstr(_link);
  if(!string_chop_string(&link, STRING("https://open.spotify.com/"))) {
    panic("wrong prefix");
  }
  string_chop_string(&link, STRING("intl-de/"));

  string prefix = {0};
  bool is_track = string_chop_string(&link, STRING("track/"));
  if(!is_track) {
    prefix = string_chop_by_delim(&link, '/');
  }

  if(is_track) {
    string name;
    if(!get_track_name(access_token, link, &temp, &name)) {
      panic("get_track_name");
    }

    printf(String_Fmt"\n", String_Arg(name));
    printf(String_Fmt"\n", String_Arg(access_token));
  } else {
    printf("TODO\n");
  }
  
  return 0;
}
