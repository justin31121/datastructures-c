#define YOUTUBE_IMPLEMENTATION
#include "../libs/youtube.h"

#define DOWNLOAD_IMPLEMENTATION
#include "../libs/download.h"

char *next(int *argc, char ***argv) {
  if((*argc) == 0) return NULL;
  char *res = *argv[0];
  (*argc)--;
  (*argv)++;
  return res;
}

String_Buffer sb = {0};

typedef enum{
	     STATE_NONE,
	     STATE_AUDIO,
	     STATE_VIDEO,
}State;

int main(int argc, char **argv) {
    
  State state = STATE_NONE;
  bool download = false;
  bool pack = false;
  const char *out_name = NULL;

  string itag = {0};
  const char *program = next(&argc, &argv);

  const char *in = next(&argc, &argv);  
  if(in == NULL) {
    fprintf(stderr, "ERROR: Please provide a searchterm or an url\n");
    fprintf(stderr, "USAGE: %s [-o <output_name>] (--pack / [-d] [ (-f <tag>) / (-audio/-video)]) <searchterm/url>\n", program);
    exit(1);
  }

  if(strcmp(in, "-o") == 0) {
    out_name = next(&argc, &argv);
    in = next(&argc, &argv);
  }

  if(strcmp(in, "--pack") == 0) {
    pack = true;
    state = STATE_AUDIO; //search for audio reguarly and video specially
    in = next(&argc, &argv);
  }

  if(!pack && strcmp(in, "-d") == 0) {
    download = true;
    in = next(&argc, &argv);
  }

  if(!pack && strcmp(in, "-f") == 0) {
    itag = string_from_cstr(next(&argc, &argv));
    in = next(&argc, &argv);
  } else if(strcmp(in, "-audio") == 0) {
    state = STATE_AUDIO;
    in = next(&argc, &argv);
  } else if(strcmp(in, "-video") == 0) {
    state = STATE_VIDEO;
    in = next(&argc, &argv);
  }
  
  if(in == NULL) {
    fprintf(stderr, "ERROR: Please provide a searchterm or an url\n");
    fprintf(stderr, "USAGE: %s [ (-f <tag>) / (-audio/-video) ] <searchterm/url>\n", program);
    exit(1);
  }

  const char *term = in;

  Http http;
  if(!http_init2(&http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
    panic("http_init2");
  }
  string_buffer_reserve(&sb, 5 * 1024 * 1024);

  string videoId;
  if(!youtube_get_videoId(term, &videoId)) {

    Youtube_Results results;
    if(!youtube_results_init(string_from_cstr(term), &http, &sb, &results)) {
      panic("youtube_results_init");
    }
    youtube_results_dump(&results);
	
    return 0;
  }

  string response;
  if(!youtube_get_response(videoId, &http, &sb, &response)) {
    panic("youtube_get_response");
  }

  Youtube_Info info;
  if(!youtube_info_init(response, &info)) {
    panic("youtube_info_init");
  }

  if(info.m3u8_source.len) {
    printf(String_Fmt"\n", String_Arg(info.m3u8_source));
    return 0;
  } else if(info.dash_source.len) {
    printf(String_Fmt"\n", String_Arg(info.dash_source));
    return 0;      
  }

  if(pack) {
    UNIMPLEMENTED();
  }

  string signature = {0};
  bool is_signature = false;
    
  if(itag.len) {	
    if(!youtube_info_find_stream(&info, itag, &signature, &is_signature)) {
      panic("youtube_info_find_stream");
    }
  } else if(state == STATE_AUDIO) {
    if(!youtube_info_find_stream(&info, STRING("140"), &signature, &is_signature)) {
      panic("youtube_info_find_audio");
    }
  } else if(state == STATE_VIDEO) {
    if(!youtube_info_find_video(&info, &signature, &is_signature)) {
      panic("youtube_info_find_video");
    }
  } else if(state == STATE_NONE) {
    youtube_info_dump(&info);
    return 0;
  }

  if(!is_signature) {

    string decoded_url = tsmap(&sb, signature, http_decodeURI);
    const char *url = tprintf(&sb, String_Fmt, String_Arg(decoded_url));
    if(download) {
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
    printf(String_Fmt"\n", String_Arg( decoded_url ));
	
    return 0;
  }

  String_Buffer temp = {0};
  Youtube_Decoder decoder;
  if(!youtube_decoder_init(response, &http, &temp, &decoder)) {
    panic("decoder");
  }

  duk_context *duk_ctx = duk_create_heap_default();

  const char* url;
  HttpHeader header;
  int i=0;
  for(;i<2;i++) {
    if(!youtube_decoder_decode(&decoder, &temp, duk_ctx, signature, &url)) {
      panic("youtube_decoder_decode");
    }

  check:
    if(!http_head(url, &header, NULL)) {
      panic("http_head");
    }
    
    int res = http_header_response_code(&header);
    if(res == 200) {
      break;
    } else if(res == 301 || res == 302 || res ==304 ) {

      string key = STRING_STATIC("Location");
      string value;
      if(!http_header_has(&header, key, &value)) {
	panic("Rediraction with code: %d has not Location", res);
      }

      url = tprintf(&temp, String_Fmt, String_Arg(value) );
      goto check;
    }
  }
  if(i == 2) {
    panic("Can not find url");
  }

  if(download) {
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

  printf("%s\n", url);
}
