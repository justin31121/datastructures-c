#define STRING_DEBUG
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

#define YOUTUBE_IMPLEMENTATION
#include "../libs/youtube.h"

#define DOWNLOAD_IMPLEMENTATION
#include "../libs/download.h"

typedef struct{
  Arr *names;
  string prev;
}Spotify_Tracks;

string SPOTIFY_TRACKS_FRAME = STRING_STATIC("sptframe");

bool on_elem_tracks(Json_Parse_Type type, string content, void *arg, void **elem) {
  (void) type;
  (void) elem;

  Spotify_Tracks *tracks = (Spotify_Tracks *) arg;
  tracks->prev = content;
  
  return true;
}

void on_object_elem_tracks(void *object, string key, void *elem, void *arg) {
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
  }
}

bool get_track_names(string access_token, string prefix, string id, String_Buffer *sb, Arr *names) {
  
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
  events.on_elem = on_elem_tracks;
  events.on_object_elem = on_object_elem_tracks;
  events.arg = &tracks;
  if(!json_parse2(response.data, response.len, &events)) {
    return false;
  }
  
  return true;
}

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

char *next(int *argc, char ***argv) {
    if((*argc) == 0) return NULL;
    char *res = *argv[0];
    (*argc)--;
    (*argv)++;
    return res;
}

String_Buffer temp = {0};

int main(int argc, char **argv) {

  const char *program = next(&argc, &argv);

  bool download = false;
  const char *out_name = NULL;

  const char *in = next(&argc, &argv);
  if(in == NULL) {
    fprintf(stderr, "ERROR: Please provide enough arguments\n");
    fprintf(stderr, "USAGE: %s [-o <output_name>] [-d] <spotfiy-link>\n", program);
    return 1;
  }  
  
  if(strcmp(in, "-o") == 0) {
    out_name = next(&argc, &argv);
    in = next(&argc, &argv);
  }

  if(strcmp(in, "-d") == 0) {
    download = true;
    in = next(&argc, &argv);
  }

  if(in == NULL) {
    fprintf(stderr, "ERROR: Please provide enough arguments\n");
    fprintf(stderr, "USAGE: %s [-o <output_name>] [-d] <spotfiy-link>\n", program);
    return 1;
  }  

  char spotify_creds[1024];
  if(!io_getenv("SPOTIFY_CREDS", spotify_creds, sizeof(spotify_creds))) {
    fprintf(stderr, "ERROR: The envorinment variable 'SPOTIFY_CREDS' is not set.\n");
    fprintf(stderr, "It should be set this way: 'clientId:clientSecrect'\n");
    return 1;
  }

  string_buffer_reserve(&temp, 1024 * 1024 * 5);

  string access_token;
  if(!get_access_token(spotify_creds, &temp, &access_token)) {
    panic("get_access_token");
  }

  string link = string_from_cstr(in);
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

    Http http;
    if(!http_init2(&http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
      panic("http_init2");
    }

    Youtube_Results results;
    if(!youtube_results_init(name, &http, &temp, &results)) {
      panic("youtube_results_init");
    }
    string videoId = *(string *) arr_get(results.videoIds, 0);

    duk_context* duk_ctx = duk_create_heap_default();

    string _url;
    if(!youtube_get_audio2(videoId, &http, &temp, duk_ctx, &_url, NULL)) {
      panic("youtube_get_audio2");
    }

    if(download) {
      const char *url = tprintf(&temp, String_Fmt, String_Arg(_url));
      
      char *out_buffer;
      size_t out_buffer_size;
      printf("Url: '%s'\n", url);
      
      download3(url, true, &out_buffer, &out_buffer_size);

      if(!out_name) {
	out_name = "videoplayback.mp4";
      }
      io_write_file_len(out_name, out_buffer, out_buffer_size);
      printf("Saved: '%s'\n", out_name);
      return 0;
    }

    printf(String_Fmt"\n", String_Arg(_url));
  } else {

    Arr *names = arr_init2(sizeof(string), 32);
    
    if(!get_track_names(access_token, prefix, link, &temp, names)) {
      panic("get_track_names");
    }

    char dir_name[1024];
    size_t dir_name_size = 0;
    char keyword[2][1024];
    size_t keyword_size = 0;
    
    char *out_buffer;
    size_t out_buffer_size = 0;
    size_t data_size;

    Http http;
    if(!http_init2(&http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
      panic("http_init2");
    }
    duk_context* duk_ctx = duk_create_heap_default();
    size_t temp_len = temp.len;
    
    if( string_eq_cstr(prefix, "album") ) {
      size_t n = names->count;
      size_t i = 0;

      string next = *(string *) arr_get(names, i++);
      bool album_name = true;

      while(i < n) {
	temp.len = temp_len;
	while( !string_eq(next, SPOTIFY_TRACKS_FRAME) ) {

	  if(keyword_size) keyword[0][keyword_size++] = ' ';
	  memcpy(keyword[0] + keyword_size, next.data, next.len );
	  keyword_size += next.len;
	  keyword[0][keyword_size] = 0;
	  
	  next = *(string *) arr_get(names, i++);	  
	}
	if(album_name) {
	  next = *(string *) arr_get(names, i-2);
	  
	  dir_name[dir_name_size++] = '.';
	  dir_name[dir_name_size++] = '\\';
	  memcpy(dir_name + dir_name_size, next.data, next.len );
	  dir_name_size+=next.len;
	  dir_name[dir_name_size] = 0;

	  io_mkdir( dir_name );
	} else {
	  next = *(string *) arr_get(names, i-2);
	  size_t size = dir_name_size;

	  dir_name[dir_name_size++] = '\\';
	  for(size_t j=0;j<next.len;j++) {
	    char c = next.data[j];
	    if(c == '/') dir_name[dir_name_size++] = '_';
	    else dir_name[dir_name_size++] = c;
	  }
	  dir_name[dir_name_size++] = '.';
	  dir_name[dir_name_size++] = 'm';
	  dir_name[dir_name_size++] = '4';
	  dir_name[dir_name_size++] = 'a';
	  
	  dir_name[dir_name_size] = 0;
	  
	  printf("%s\n", dir_name);
	  dir_name_size = size;

	  //////////////////////////////////////////
	  
	  size = keyword_size;
	  size_t output_size;
	  if(!http_encodeURI(keyword[0], keyword_size,
			     keyword[1], sizeof(keyword) / 2,
			     &output_size)) {
	    panic("keyword overflow");
	  }
	  keyword[1][output_size] = 0;

	  //////////////////////////////////////////	  

	  Youtube_Results results;
	  if(!youtube_results_init(string_from_cstr(keyword[1]), &http, &temp, &results)) {
	    panic("youtube_results_init");
	  }
	  string videoId = *(string *) arr_get(results.videoIds, 0);

	  string _url;
	  if(!youtube_get_audio2(videoId, &http, &temp, duk_ctx, &_url, NULL)) {
	    panic("youtube_get_audio2");
	  }
	  youtube_results_free(&results);

	  const char *url = tprintf(&temp, String_Fmt, String_Arg(_url) );

	  printf("Url: '%s'\n", url);

	  if(out_buffer_size == 0) {
	    download3(url, true, &out_buffer, &out_buffer_size);
	    data_size = out_buffer_size;
	  } else {
	    size_t new_out_buffer_size;
	    if(!download_get_size(url, &new_out_buffer_size)) {
	      panic("download_get_size");
	    }

	    if(new_out_buffer_size > out_buffer_size) {
	      out_buffer_size = new_out_buffer_size;
	      out_buffer = realloc(out_buffer, out_buffer_size);
	      if(!out_buffer) {
		panic("not enough memory");
	      }
	    }

	    if(!download2(url, true, out_buffer, out_buffer_size, &data_size)) {
	      panic("download2");
	    }
	  }
	  	  	  
	  if(!io_write_file_len(dir_name, out_buffer, data_size)) {
	    warn("failed to write: %s\n", dir_name);
	  }
	  printf("Saved: '%s'\n", dir_name);
	  
	}

	keyword_size = 0;
	next = *(string *) arr_get(names, i++);	
	album_name = false;	
      }
      
    }
    
  }
  
  return 0;
}
