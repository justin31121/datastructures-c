#define HTTP_IMPLEMENTATION
#include "../libs/http.h"

#define IO_IMPLEMENTATION
#include "../libs/io.h"

#define BASE64_IMPLEMENTATION
#include "../libs/base64.h"

#define JSON_PARSER_IMPLEMENTATION
#include "../libs/json_parser.h"

#define YOUTUBE_IMPLEMENTATION
#include "../libs/youtube.h"

#define DOWNLOAD_IMPLEMENTATION
#include "../libs/download.h"

#define SPOTIFY_IMPLEMENTATION
#include "../libs/spotify.h"

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
  bool m3u8_playlist = false;
  const char *out_name = NULL;

  const char *in = next(&argc, &argv);
  if(in == NULL) {
    fprintf(stderr, "ERROR: Please provide enough arguments\n");
    fprintf(stderr, "USAGE: %s [-o <output_name>] [-d / -m] <spotfiy-link>\n", program);
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

  if(strcmp(in, "-m") == 0) {
    m3u8_playlist = true;
    in = next(&argc, &argv);
  }

  if(in == NULL) {
    fprintf(stderr, "ERROR: Please provide enough arguments\n");
    fprintf(stderr, "USAGE: %s [-o <output_name>] [-d] <spotfiy-link>\n", program);
    return 1;
  }

  if(download && m3u8_playlist) {
    fprintf(stderr, "ERROR: Either download or create m3u8-playlist\n");
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
  if(!spotify_get_access_token(spotify_creds, &temp, &access_token)) {
    panic("get_access_token");
  }

  string link = string_from_cstr(in);
  if(!string_chop_string(&link, STRING("https://open.spotify.com/"))) {
    Spotify_Results results;
    if(!spotify_results_init(access_token, link, &temp, &results)) {
      panic("spotify_results_init");
    }

    spotify_results_dump(&results);
    return 0;
  }
  string_chop_string(&link, STRING("intl-de/"));

  string prefix = {0};
  bool is_track = string_chop_string(&link, STRING("track/"));
  if(!is_track) {
    prefix = string_chop_by_delim(&link, '/');
  }

  if(is_track) {
    string name;
    if(!spotify_get_track_name(access_token, link, &temp, &name)) {
      panic("get_track_name");
    }
    printf(String_Fmt"\n", String_Arg(name)); fflush(stdout);

    Http http;
    if(!http_init2(&http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
      panic("http_init2");
    }

    string videoId;
    if(!youtube_results_first(name, &http, &temp, &videoId)) {
      panic("youtube_results_first");
    }
    duk_context* duk_ctx = duk_create_heap_default();
    //printf(String_Fmt"\n", String_Arg(videoId));

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

      name = tsreplace(&temp, name, "/", "_");
      const char* filename;
      if(out_name) {
	filename = tprintf(&temp, "%s/"String_Fmt".m4a", out_name, String_Arg(name));
      } else {
	filename = tprintf(&temp, String_Fmt".m4a", String_Arg(name));
      }

      if(io_write_file_len(filename, out_buffer, out_buffer_size)) {
	printf("INFO: Saved: '%s'\n", filename);
      } else {
	printf("WARNING: Failed to save: '%s'\n", filename);
      }
      
      return 0;
    }
	
    printf(String_Fmt"\n", String_Arg(_url));
  } else {

    char *out_buffer = NULL;
    size_t out_buffer_size = 0;
    size_t out_buffer_cap = 0;

    //TODO: add download support back
    Http http;
    if(!http_init2(&http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
      panic("http_init2");
    }
    duk_context* duk_ctx = duk_create_heap_default();
	
    Spotify_Tracks tracks;
    if(!spotify_get_track_names(access_token, prefix, link, &temp, &tracks)) {
      panic("get_track_names");
    }

    string folder = spotify_tracks_get_folder(&tracks);
    if(!download) printf( String_Fmt"\n", String_Arg(folder) );
    else {
      if(out_name) {
	io_mkdir( tprintf(&temp, "%s/"String_Fmt, out_name, String_Arg(folder)) );
      } else {
	io_mkdir( tprintf(&temp, String_Fmt, String_Arg(folder)) );
      }
    }

    FILE *file;
    if(m3u8_playlist) {
      file = fopen(out_name, "wb");
      if(file == NULL) {
	fprintf(stderr, "ERROR: Can not open '%s'\n", out_name);
	return 1;
      }

      fprintf(file, "#EXTM3U\n\n");
    }
    
    size_t temp_len = temp.len;
    char name_buf[1024];
    size_t name_size, name_off;
    while( spotify_tracks_next(&tracks, name_buf, sizeof(name_buf), &name_size, &name_off) ) {
      temp.len = temp_len;

      string name = string_from(name_buf, name_size);
      string short_name = string_from(name_buf + name_off, name_size - name_off);
      printf( String_Fmt" ("String_Fmt")\n", String_Arg(name), String_Arg(short_name) );

      if(download) {
	      
	const char* filename;
	const char* dir_name;
	short_name = tsreplace(&temp, short_name, "/", "_");
	if(out_name) {
	  dir_name = tprintf(&temp, "%s/"String_Fmt, out_name);
	} else {
	  dir_name = tprintf(&temp, "./"String_Fmt"/", String_Arg(folder));
	}
	filename = tprintf(&temp, "%s"String_Fmt".m4a", dir_name, String_Arg(short_name));

	bool is_file;
	if(!io_exists(dir_name, &is_file)) {
	  if(!io_mkdir(dir_name)) {
	    panic("io_mkdir");
	  }
	}

	if(!io_exists(filename, &is_file)) {
	  string videoId;
	  if(!youtube_results_first(name, &http, &temp, &videoId)) {
	    panic("youtube_results_first");
	  }

	  string _url;
	  if(!youtube_get_audio2(videoId, &http, &temp, duk_ctx, &_url, NULL)) {
	    panic("youtube_get_audio2");
	  }

	  const char *url = tprintf(&temp, String_Fmt, String_Arg(_url) );
	  printf("Url: '%s'\n", url); fflush(stdout);

	  if(out_buffer == NULL) {
	    download3(url, true, &out_buffer, &out_buffer_size);
	    out_buffer_cap = out_buffer_size;
	  } else {
	    size_t data_size;
	    download_get_size(url, &data_size);
	    if(out_buffer_cap < data_size) {
	      out_buffer_cap = data_size;
	      out_buffer = realloc(out_buffer, data_size);
	      if(!out_buffer_cap) {
		panic("Can not allocate enough memory");
	      }
	    }

	    download2(url, true, out_buffer, out_buffer_cap, &out_buffer_size);
	  }

	  if(io_write_file_len(filename, out_buffer, out_buffer_size)) {
	    printf("INFO: Saved: '%s'\n", filename);
	  } else {
	    printf("WARNING: Failed to save: '%s'\n", filename);
	  }
		
	}
      } else if(m3u8_playlist) {
	
	string videoId;
	if(!youtube_results_first(name, &http, &temp, &videoId)) {
	  panic("youtube_results_first");
	}

	string _url;
	if(!youtube_get_audio2(videoId, &http, &temp, duk_ctx, &_url, NULL)) {
	  panic("youtube_get_audio2");
	}

	
	printf(String_Fmt"\n", String_Arg(short_name)); fflush(stdout);
	fprintf(file, "#EXTINF:,"String_Fmt"\n"String_Fmt"\n\n", String_Arg(short_name), String_Arg(_url));
	
      } else {
	string videoId;
	if(!youtube_results_first(name, &http, &temp, &videoId)) {
	  panic("youtube_results_first");
	}

	string _url;
	if(!youtube_get_audio2(videoId, &http, &temp, duk_ctx, &_url, NULL)) {
	  panic("youtube_get_audio2");
	}

	const char *url = tprintf(&temp, String_Fmt, String_Arg(_url) );
	printf("Url: '%s'\n", url); fflush(stdout);
      }
    }

    if(m3u8_playlist) {
      fclose(file);
    }


  }


  /*
    Arr *names = arr_init2(sizeof(string), 32);
    
    if(!spotify_get_track_names(access_token, prefix, link, &temp, names)) {
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

    bool is_playlist = string_eq_cstr(prefix, "playlist");
    
    size_t n = names->count;
    size_t i = 0;

    string next = *(string *) arr_get(names, i++);
    bool album_name = !is_playlist;
    if(is_playlist) {
    dir_name[dir_name_size++] = '.';
    dir_name[dir_name_size++] = '\\';
    memcpy(dir_name + dir_name_size, next.data, next.len );
    dir_name_size+=next.len;
    dir_name[dir_name_size] = 0;

    if(download) io_mkdir( dir_name );
    next = *(string *) arr_get(names, i++);
    }

    while(i < n) {
    temp.len = temp_len;

    while(is_playlist && !string_eq(next, SPOTIFY_TRACKS_FRAME) ) {
    printf( "skipping: '"String_Fmt"'\n", String_Arg(next) );
    next = *(string *) arr_get(names, i++);	
    }
    if(is_playlist) {
    next = *(string *) arr_get(names, i++);
    }
	    
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

    if(download) io_mkdir( dir_name );
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
	  
    printf("Dir_name: '%s'\n", dir_name); fflush(stdout);
    dir_name_size = size;

    printf("Name: '%s'\n", keyword[0]); fflush(stdout);

    //////////////////////////////////////////

    string videoId;
    if(!youtube_results_first(string_from_cstr(keyword[0]), &http, &temp, &videoId)) {
    panic("youtube_results_first");
    }

    string _url;
    if(!youtube_get_audio2(videoId, &http, &temp, duk_ctx, &_url, NULL)) {
    panic("youtube_get_audio2");
    }

    const char *url = tprintf(&temp, String_Fmt, String_Arg(_url) );

    printf("Url: '%s'\n", url); fflush(stdout);

    if(download) {
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
    }
		
    printf("Saved: '%s'\n", dir_name); fflush(stdout);
	  
    }

    keyword_size = 0;
    next = *(string *) arr_get(names, i++);
    album_name = false;	
    }
  */
  return 0;
}
