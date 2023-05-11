#include <stdio.h>

#define JSON_PARSER_IMPLEMENTATION
#include "../libs/json_parser.h"

#define YOUTUBE_IMPLEMENTATION
#include "../libs/youtube.h"

#define HTML_PARSER_IMPLEMENTATION
#include "../libs/html_parser.h"

#define REGEX_IMPLEMENTATION
#include "../libs/regex.h"

#define DOWNLOAD_IMPLEMENTATION
#include "../libs/download.h"

#define PREFIX "var ytInitialPlayerResponse = "

typedef struct{
  string dash_source;
  string m3u8_source;
  string prev;
  string prev_prev;
  int state;
  Arr *strings;
}Player_Info;

bool on_elem(Json_Parse_Type type, string content, void *arg, void **elem) {
  (void) type;
  (void) elem;
  Player_Info *foo = (Player_Info *) arg;

  //printf(String_Fmt"\n", String_Arg(content));
    
  if(foo->state == 0) {
    if( (string_index_of(content, "video/mp4") >= 0)  ||
	(string_index_of(content, "video/webm") >= 0) ||
	(string_index_of(content, "audio/mp4") >= 0)  ||
	(string_index_of(content, "audio/webm") >= 0) ) {
      if(string_index_of(foo->prev, "https://") == 0) {
	arr_push(foo->strings, &foo->prev_prev);
	arr_push(foo->strings, &content);
	arr_push(foo->strings, &foo->prev);
      } else {
	foo->state = 1;
	arr_push(foo->strings, &foo->prev);
	arr_push(foo->strings, &content);
      }      
    }
  } else if(foo->state == 1) {
    if(string_index_of(content, "s=") == 0) {
      arr_push(foo->strings, &content);
      foo->state = 0;
    }
  }

  foo->prev_prev = foo->prev;
  foo->prev = content;
  return true;
}

void on_object_elem(void *object, string key, void *elem, void *arg) {
  (void) object;
  (void) key;
  (void) elem;
  Player_Info *foo = (Player_Info *) arg;

  if(string_eq(key, STRING("hlsManifestUrl"))) {
    foo->m3u8_source = foo->prev;
  } else if(string_eq(key, STRING("dashManifestUrl"))) {
    foo->dash_source = foo->prev;
  }
}

void on_node_content(void *_node, string content, void *arg) {
  (void) _node;
  if(string_index_of(content, PREFIX) != 0) return;
  *((string *) arg) = content;
}

//Will discard everything allocated in sb
//Returns json stored in sb
bool query_json(Http *http, const char *url, String_Buffer *sb, string *initialPlayerResponseString) {

  sb->len = 0;
  const char *route = url + 8 + strlen(YOUTUBE_HOSTNAME);
  if(!http_request(http, route,
		   "GET", NULL, NULL, string_buffer_callback, sb, NULL, NULL)) {
    return false;
  }
    
  string json_string;  
  Html_Parse_Events events = {0};
  events.on_node_content = on_node_content;
  events.arg = &json_string;
  
  if(!html_parse(sb->data, sb->len, &events)) {
    return false;
  }

  string_chop_left(&json_string, strlen(PREFIX)); // 'var ytInitialData = '
  string_chop_right(&json_string, 1); // ';'

  *initialPlayerResponseString = json_string;
  return true;
}

void search(const char *term) {
  char buf[1024];
  size_t buf_size;
  if(!http_encodeURI(term, strlen(term), buf, 1023, &buf_size)) {
    panic("buf overflow");
  }
  buf[buf_size] = 0;

  Youtube_Context context;
  if(!youtube_context_init(&context)) {
    panic("youtube_context_init");
  }
  if(!youtube_context_start(&context)) {
    panic("youtube_context_start");
  }

  Json out;
  if(!youtube_search(&context, buf, &out)) {
    panic("youtube_search");
  }

  for(int i=0;i<json_size(out);i++) {
    Json result = json_opt(out, i);
    if(result.type != JSON_OBJECT) {
      continue;
    }

    const char *videoId = json_get_string(result, "videoId");
    const char *title = json_get_string(
					json_opt(json_get(json_get(result, "title"), "runs"), 0), "text");
       
    printf("'%s' https://www.youtube.com/watch?v=%s\n", title, videoId);
  }
}

bool get_streams(Player_Info *info, Http *http, String_Buffer *sb, const char *youtube_url) {
  string response;
  if(!query_json(http, youtube_url, sb, &response)) {
    return false;
  }
 
  info->state = 0;
  info->strings = arr_init(sizeof(string));
  info->dash_source = (string) {0};
  info->m3u8_source = (string) {0};

  Json_Parse_Events events = {0};
  events.on_elem = on_elem;
  events.on_object_elem = on_object_elem;
  events.arg = info;

  if(!json_parse2(response.data, response.len, &events)) {
    return false;
  }

  return true;
}

bool find_stream(Player_Info *info, string itag, string *stream, bool *is_signature) {
  *stream = (string) {0};
  for(size_t i=0;i<info->strings->count;i+=3) {
    string tag = *(string *) arr_get(info->strings, i);
    string _signature = *(string *) arr_get(info->strings, i+2);
    if(string_eq(tag, itag)) {
      *stream = _signature;
      *is_signature = !(string_index_of(*stream, "https://") == 0);
    }
  }

  return stream->len;
}

typedef struct{
  String_Buffer buffer;
  duk_context* duk_ctx;
  const char *decodeFunction;
  string prefix;
}Decoder;

bool build_decoder(Decoder *decoder, Http *http, string response_string) {

  decoder->buffer = (String_Buffer) {0};
  decoder->duk_ctx = duk_create_heap_default();

  Regex regexs[4] = {0};
  if(!regex_compile(&regexs[0], "/s/player/[:alnum:]+/[:alnum_.:]+/[:alnum_:]+/base\\.js")) {
    panic("can not copmile regex");
  }
  if(!regex_compile(&regexs[1], "function[:print:]*\\.split\\(\"\"\\)[:print:]*\\.join\\(\"\"\\)}")) {
    panic("can not copmile regex");
  }
  if(!regex_compile(&regexs[2], "\\.split\\(\"\"\\);[:alnum:]+\\.")) {
    panic("can not copmile regex");
  }

  size_t offset, len;	
  //file
  if(!regex_match_len(&regexs[0], response_string.data, response_string.len, &offset, &len)) {
    panic("regex_match");
  }

  const char *js_url = tprintf(&decoder->buffer, "%.*s", (int) len, response_string.data + offset);
  decoder->buffer.len = 0;
  if(!http_request(http, js_url,
		   "GET", NULL, NULL, string_buffer_callback, &decoder->buffer, NULL, NULL)) {
    panic("http_get");
  }
  size_t buffer_len = decoder->buffer.len;

  //decode function
  if(!regex_match_len(&regexs[1], decoder->buffer.data, buffer_len, &offset, &len)) {
    panic("regex_match");
  }

  decoder->decodeFunction = tprintf(&decoder->buffer, "%.*s", (int) len, decoder->buffer.data + offset);
  // -- varNameMatches
  if(!regex_match(&regexs[2], decoder->decodeFunction, &offset, &len)) {
    panic("regex_match");
  }
  len -= 1 + 11;
  offset += 11;

  const char *varDeclareMatches = tprintf(&decoder->buffer, "var %.*s={.+}};[:alnum:]+\\.[:alnum:]+\\.prototype",
					  (int) len, decoder->decodeFunction + offset);
  if(!regex_compile(&regexs[3], varDeclareMatches)) {
    panic("can not copmile regex");
  }
  if(!regex_match_len(&regexs[3], decoder->buffer.data, buffer_len, &offset, &len)) {
    panic("regex_match");
  }

  int k = len-1;
  while(k>=0 && decoder->buffer.data[offset+k] != '}') k--;
  len = (size_t) k+2;

  decoder->prefix = string_from(decoder->buffer.data + offset, len);

  return true;
}

bool decode(Decoder *decoder, string signature, const char **out_url) {

  string_chop_by_delim(&signature, '=');
  string _s = string_chop_by_delim(&signature, '=');
  string __s = string_from(_s.data, _s.len - 8);
  string url = {0};

  while(signature.len) {
    string param = string_chop_by_delim(&signature, '=');
    if(string_index_of(param, "https:") != 0) continue;
    url = param;
  }

  if(!url.len) {
    return false;
  }

  const char *expression = tprintf(&decoder->buffer, "\"use-strict\";"String_Fmt"encodeURIComponent((%s)(decodeURIComponent(\""String_Fmt"\")));",
				   String_Arg(decoder->prefix), decoder->decodeFunction, String_Arg(__s));
  duk_eval_string(decoder->duk_ctx, expression);
	
  char decoded_url[1024];
  size_t decoded_url_size;    
  if(!http_decodeURI(url.data, url.len, decoded_url, 1023, &decoded_url_size)) {
    panic("decoded_url overflow");
  }
  decoded_url[decoded_url_size] = 0;

  *out_url = tprintf(&decoder-> buffer, "%s&sig=%s", decoded_url, duk_get_string(decoder->duk_ctx, -1));
  
  return true;
}

String_Buffer sb = {0};
String_Buffer buffer = {0};

char *next(int *argc, char ***argv) {
  if((*argc) == 0) return NULL;
  char *res = *argv[0];
  (*argc)--;
  (*argv)++;
  return res;
}

typedef enum{
	     STATE_NONE,
	     STATE_AUDIO,
	     STATE_VIDEO,
}State;

int main(int argc, char **argv) {

  State state = STATE_NONE;
  bool download = false;
  const char *out_name = NULL;

  string itag = {0};
  const char *program = next(&argc, &argv);

  const char *in = next(&argc, &argv);  
  if(in == NULL) {
    fprintf(stderr, "ERROR: Please provide a searchterm or an url\n");
    fprintf(stderr, "USAGE: %s [-o <output_name>] [-d] [ (-f <tag>) / (-audio/-video) ] <searchterm/url>\n", program);
    exit(1);
  }

  if(strcmp(in, "-o") == 0) {
    out_name = next(&argc, &argv);
    in = next(&argc, &argv);
  }

  if(strcmp(in, "-d") == 0) {
    download = true;
    in = next(&argc, &argv);
  }

  if(strcmp(in, "-f") == 0) {
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

  string videoId;
  if(youtube_get_videoId(term, &videoId)) {
    
    Http http;
    if(!http_init2(&http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
      panic("http_init2");
    }

    Player_Info info;
    if(!get_streams(&info, &http, &sb, term)) {
      panic("get_streams");
    }

    if(info.m3u8_source.len) {
      printf(String_Fmt"\n", String_Arg(info.m3u8_source));
      return 0;
    } else if(info.dash_source.len) {
      printf(String_Fmt"\n", String_Arg(info.dash_source));
      return 0;      
    }

    if(!itag.len) {
      
      for(size_t i=info.strings->count-1;i>=3;i-=3) {
	string tag = *(string *) arr_get(info.strings, i-2);
	string type = *(string *) arr_get(info.strings, i-1);
	if(state == STATE_NONE) {
	  printf(String_Fmt" - "String_Fmt"\n", String_Arg(tag), String_Arg(type));
	} else if(state == STATE_AUDIO) {
	  if(string_index_of(type, "audio/") == 0) {
	    itag = tag;
	  }
	} else if(state == STATE_VIDEO) {
	  if(string_index_of(type, "video/") == 0) {
	    itag = tag;
	  }
	}
      }
      
    }

    if(!itag.len) {
      
      if(download) {
	fprintf(stderr, "ERROR: Specifiy a stream or use -audio/-video tags, for downloading a resource\n");
	return 1;
      }
      
      return 0;
    }

    const char *url;
    string stream;
    bool is_signature;
    if(!find_stream(&info, itag, &stream, &is_signature)) {
      panic("find_stream");
    }

    if(!is_signature) {
      char decoded_url[1024 * 2];
      size_t decoded_url_size;
      if(!http_decodeURI(stream.data, stream.len, decoded_url, 1023 * 2, &decoded_url_size)) {
	panic("decoded_url overflow");
      }
      decoded_url[decoded_url_size] = 0;

      url = decoded_url;
    } else {
      Decoder decoder;
      if(!build_decoder(&decoder, &http, string_from(sb.data, sb.len))) {
	panic("build decoder");
      }

      if(!decode(&decoder, stream, &url)) {
	panic("decode");
      }
    }

    if(download) {
      char *out_buffer;
      size_t out_buffer_size;
      printf("Downloading: '%s'\n", url);
      download3(url, &out_buffer, &out_buffer_size);

      if(!out_name) {
	out_name = "videoplayback.mp4";
      }
      write_file_len(out_name, out_buffer, out_buffer_size);
      printf("Saved: '%s'\n", out_name);
    } else {
      printf("%s\n", url);
    }
     
  } else {
    search(term);
  }
  
  return 0;
}
