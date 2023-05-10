#include <stdio.h>

#define JSON_PARSER_IMPLEMENTATION
#include "../libs/json_parser.h"

#define YOUTUBE_IMPLEMENTATION
#include "../libs/youtube.h"

#define HTML_PARSER_IMPLEMENTATION
#include "../libs/html_parser.h"

#define REGEX_IMPLEMENTATION
#include "../libs/regex.h"

#define PREFIX "var ytInitialPlayerResponse = "

typedef struct{
  bool verbose;
  string prev;
  int state;
  Arr *signatures;
  Arr *tags;
}Foo;

bool on_elem(Json_Parse_Type type, string content, void *arg, void **elem) {
  (void) type;
  (void) elem;
  Foo *foo = (Foo *) arg;
  //printf("%s - "String_Fmt"\n", json_parse_type_name(type), String_Arg(content));
    
  if(foo->state == 0) {
    if( (string_index_of(content, "video/mp4") >= 0)  ||
	(string_index_of(content, "video/webm") >= 0) ||
	(string_index_of(content, "audio/mp4") >= 0)  ||
	(string_index_of(content, "audio/webm") >= 0) ) {
      foo->state = 1;
      arr_push(foo->tags, &foo->prev);
      if(foo->verbose) printf(""String_Fmt"\n", String_Arg(foo->prev));
    }
  } else if(foo->state == 1) {
    if(string_index_of(content, "s=") == 0) {
      arr_push(foo->signatures, &content);
      foo->state = 0;
    }
  }

  foo->prev = content;
    
  return true;
}

void on_object_elem(void *object, string key, void *elem, void *arg) {
  (void) object;
  (void) key;
  (void) elem;
  Foo *foo = (Foo *) arg;

  if(foo->state == 1) {
    printf("\t"String_Fmt": "String_Fmt"\n", String_Arg(key), String_Arg(foo->prev));
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

String_Buffer sb = {0};
String_Buffer buffer = {0};

int main(int argc, char **argv) {

  if(argc < 2) {
    fprintf(stderr, "ERROR: Please provide a searchterm\n");
    fprintf(stderr, "USAGE: %s <searchterm>\n", argv[0]);
    exit(1);
  }

  bool verbose = false;

  string videoId;
  if(youtube_get_videoId(argv[1], &videoId)) {
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



    Http http;
    if(!http_init2(&http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
      panic("http_init2");
    }
	
    string response;
    if(!query_json(&http, argv[1], &sb, &response)) {
      panic("query_json");
    }

    ///////////////////////////////////////////////////////////////

    string_buffer_append(&sb, "\0", 1);
    size_t offset, len;
	
    //file
    if(!regex_match(&regexs[0], sb.data, &offset, &len)) {
      panic("regex_match");
    }

    const char *js_url = tprintf(&buffer, "%.*s", (int) len, sb.data + offset);	
    buffer.len = 0;
    if(!http_request(&http, js_url,
		     "GET", NULL, NULL, string_buffer_callback, &buffer, NULL, NULL)) {
      panic("http_get");
    }
    string_buffer_append(&buffer, "\0", 1);


    //decode function
    if(!regex_match(&regexs[1], buffer.data, &offset, &len)) {
      panic("regex_match");
    }


    const char *decodeFunction = tprintf(&buffer, "%.*s", (int) len, buffer.data + offset);

    // -- varNameMatches
    if(!regex_match_nth(&regexs[2], decodeFunction, 0, &offset, &len)) {
      panic("regex_match");
    }
    len -= 1 + 11;
    offset += 11;

    const char *varDeclareMatches = tprintf(&buffer, "var %.*s={.+}};[:alnum:]+\\.[:alnum:]+\\.prototype",
					    (int) len, decodeFunction + offset);
    if(!regex_compile(&regexs[3], varDeclareMatches)) {
      panic("can not copmile regex");
    }
    if(!regex_match_nth(&regexs[3], buffer.data, 0, &offset, &len)) {
      panic("regex_match");
    }


    int k = len-1;
    while(k>=0 && buffer.data[offset+k] != '}') k--;
    len = (size_t) k+2;

    ///////////////////////////////////////////////////////////////

    Foo foo = {
      .verbose = verbose,
      .state = 0,
      .signatures = arr_init(sizeof(string)),
      .tags = arr_init(sizeof(string))
    };


    Json_Parse_Events events = {0};  
    events.on_elem = on_elem;
    if(verbose) {
      events.on_object_elem = on_object_elem;
    }
    events.arg = &foo;
  
    if(!json_parse2(response.data, response.len, &events)) {
      panic("json_parse");
    }

    string signature = {0};
    for(size_t i=0;i<foo.tags->count;i++) {
      string tag = *(string *) arr_get(foo.tags, i);
      string _signature = *(string *) arr_get(foo.signatures, i);
      if(string_eq(tag, STRING("249"))) {
	signature = _signature;
      }
      //printf(String_Fmt"\n", String_Arg(tag));
      //printf(String_Fmt"\n", String_Arg(signature));
    }
    if(!signature.len) {
      //no tag is found
    }

    string_chop_by_delim(&signature, '=');
    string _s = string_chop_by_delim(&signature, '=');
    string __s = string_from(_s.data, _s.len - 8);
    string url = {0};

    while(signature.len) {
      string param = string_chop_by_delim(&signature, '=');
      if(string_index_of(param, "https:") != 0) continue;
      url = param;
    }

    duk_context *duk_ctx = duk_create_heap_default();
    const char *expression = tprintf(&buffer,
				     "\"use-strict\";%.*sencodeURIComponent((%s)(decodeURIComponent(\""String_Fmt"\")));",
				     (int) len, buffer.data + offset, decodeFunction, String_Arg(__s));
    duk_eval_string(duk_ctx, expression);
	
    char decoded_url[1024];
    size_t decoded_url_size;
    if(!http_decodeURI(url.data, url.len, decoded_url, 1023, &decoded_url_size)) {
      panic("decoded_url overflow");
    }
    decoded_url[decoded_url_size] = 0;

    printf("%s\n", tprintf(&buffer, "%s&sig=%s", decoded_url, duk_get_string(duk_ctx, -1)) ); fflush(stdout);
  } else {
    search(argv[1]);
  }
    
  return 0;
}
