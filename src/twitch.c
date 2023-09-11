#include <stdio.h>

#define HTTP_IMPLEMENTATION
//#define HTTP_DEBUG
#include "../libs/http.h"

#define JSON_PARSER_IMPLEMENTATION
#include "../libs/json_parser.h"

#define CLIENT_ID "kimne78kx3ncx6brgo4mv6wki5h1ko"

typedef struct{
  string prev;
  string value;
  string signature;
}Foo;

bool on_elem(Json_Parse_Type type, string content, void *arg, void **elem) {
  (void) type;
  (void) arg;
  (void) elem;
  Foo *foo = (Foo *) arg;
  foo->prev = content;
  return true;
}

void on_object_elem(void *object, string key, void *elem, void *arg) {
  (void) object;
  (void) key;
  (void) elem;
  Foo *foo = (Foo *) arg;
  if(string_eq_cstr(key, "value")) {
    foo->value = foo->prev;
  } else if(string_eq_cstr(key, "signature")) {
    foo->signature = foo->prev;
  }
}

int stream_from_id(String_Buffer *temp, string id) {
  
  temp->len = 0;
  const char *token_kind = "video";
  const char *param_name = "id";
  
  const char *query_value = tprintf(temp,"[{\"query\":\"{\\n%sPlaybackAccessToken(\\n%s: \\\""String_Fmt"\\\",\\nparams: {platform: \\\"web\\\",\\nplayerBackend: \\\"mediaplayer\\\", \\nplayerType: \\\"site\\\"\\n}\\n)\\n{value\\nsignature\\n}\\n}\"}]", token_kind, param_name, String_Arg(id));
  const char *client_id_extra = tprintf(temp, "Client-ID: %s\r\n", CLIENT_ID);
  temp->len = 0;

  if(!http_post("https://gql.twitch.tv/gql", query_value, "text/plain;charset=UTF-8", string_buffer_callback, temp, NULL, client_id_extra)) {
    fprintf(stderr, "ERROR: Failed to do a post request\n");
    return 1;
  }
  Foo foo = {0};

  printf("'%.*s'\n", (int) temp->len, temp->data );

  Json_Parse_Events events = {0};
  events.on_elem = on_elem;
  events.on_object_elem = on_object_elem;
  events.arg = &foo;

  if(!json_parse2(temp->data, temp->len, &events)) {
    fprintf(stderr, "ERROR: Failed to parse json response\n");
    return 1;
  }

  if(!foo.value.len || !foo.signature.len) {
    fprintf(stderr, "ERROR: Can not find value and signature from response\n");
    return 1;
  }
  
  char buf[1024];
  size_t buf_size;
  if(!string_replace(foo.value, "\\\"", "\"", buf, 1024, &buf_size)) {
    return 1;
  }
  buf[buf_size] = 0;
  
  temp->len = 0; //flush buffer
  const char *url = tprintf(temp, "https://usher.ttvnw.net/vod/"String_Fmt".m3u8?nauthsig="String_Fmt"&allow_spectre=true&playlist_include_framerate=true&allow_audio_only=true&nauth=%s&allow_source=true&player=twitchweb", String_Arg(id), String_Arg(foo.signature), buf);

  printf("%s\n", url);

  return 0;
}

int stream_from_channel(String_Buffer *temp, string channel) {

  temp->len = 0;
  const char *token_kind = "stream";
  const char *param_name = "channelName";
  
  const char *query_value = tprintf(temp,"[{\"query\":\"{\\n%sPlaybackAccessToken(\\n%s: \\\""String_Fmt"\\\",\\nparams: {platform: \\\"web\\\",\\nplayerBackend: \\\"mediaplayer\\\", \\nplayerType: \\\"site\\\"\\n}\\n)\\n{value\\nsignature\\n}\\n}\"}]", token_kind, param_name, String_Arg(channel));
  const char *client_id_extra = tprintf(temp, "Client-ID: %s\r\n", CLIENT_ID);
  temp->len = 0;

  if(!http_post("https://gql.twitch.tv/gql", query_value, "text/plain;charset=UTF-8", string_buffer_callback, temp, NULL, client_id_extra)) {
    fprintf(stderr, "ERROR: Failed to do a post request\n");
    return 1;
  }

  Foo foo = {0};

  Json_Parse_Events events = {0};
  events.on_elem = on_elem;
  events.on_object_elem = on_object_elem;
  events.arg = &foo;

  if(!json_parse2(temp->data, temp->len, &events)) {
    fprintf(stderr, "ERROR: Failed to parse json response\n");
    return 1;
  }

  if(!foo.value.len || !foo.signature.len) {
    fprintf(stderr, "ERROR: Can not find value and signature from response\n");
    return 1;
  }

  char buf[1024];
  size_t buf_size;
  if(!string_replace(foo.value, "\\\"", "\"", buf, 1024, &buf_size)) {
    return 1;
  }
  buf[buf_size] = 0;
  
  temp->len = 0; //flush buffer
  const char *url = tprintf(temp, "https://usher.ttvnw.net/api/channel/hls/"String_Fmt".m3u8?allow_source=true&allow_audio_only=true&p=0&allow_spectre=true&player=twitchweb&playlist_include_framerate=true&segment_preference=4&sig="String_Fmt"&token=%s", String_Arg(channel), String_Arg(foo.signature), buf);

  printf("%s\n", url);
  
  return 0;
}

bool worst_stream(string m3u8string, string *url) {
  
  if(!string_chop_string(&m3u8string, STRING("#EXTM3U\n") )) {
    return false;
  }

  string last = {0};

  while(m3u8string.len) {
    string line = string_chop_by_delim(&m3u8string, '\n');
    string directive = string_chop_by_delim(&line, ':');

    if(string_eq_cstr(directive, "#EXT-X-TWITCH-INFO")) {
      //printf( String_Fmt"\n", String_Arg(line) );
      continue;
    } else if(!string_eq_cstr(directive, "#EXT-X-MEDIA")) {
      printf("Bad M3U File!\n");
      return false;
    }    

    line = string_chop_by_delim(&m3u8string, '\n');
    directive = string_chop_by_delim(&line, ':');

    if(!string_eq_cstr(directive, "#EXT-X-STREAM-INF")) {
      printf("Bad M3U Content\n");
      return false;
    }
    
    while(line.len) {
      
      string key = string_chop_by_delim(&line, '=');
      (void) key;
      string value;
      (void) value;
      if(line.len && line.data[0] == '\"') {
	value = string_chop_parenthesis(&line);
	string_chop_left(&line, 1);
      } else {
	value = string_chop_by_delim(&line, ',');
      }	

    }

    line = string_chop_by_delim(&m3u8string, '\n');
    last = line;
  }

  if(last.len == 0)
    return false;

  *url = last;
  return true;
}

typedef struct{
  string curr_copy;
  string curr;

  string prefix;
}M3u8;

bool m3u8_init(String_Buffer *temp, string m3u8_url, M3u8* m) {
  
  size_t temp_len = temp->len;
  if(!http_get(string_to_cstr(m3u8_url), string_buffer_callback, temp, NULL, NULL)) {
    return false;
  }
  m->curr = string_from(temp->data, temp->len);
  temp->len = temp_len;  
    
  int last = -1;
  string url_copy = m3u8_url;
  while(url_copy.len) {
    string path = string_chop_by_delim(&url_copy, '/');
    last = path.data - m3u8_url.data;
  }

  if(last < 0)
    return false;

  m->prefix = string_from(m3u8_url.data, (size_t) last);
  
  if(!string_chop_string(&m->curr, STRING("#EXTM3U\n") ))
    return false;

  string extinf = STRING("#EXTINF");

  int pos = string_index_of2(m->curr, extinf);
  if(pos < 0)
    return false;

  string_chop_left(&m->curr, (size_t) pos);
  m->curr_copy = m->curr;
  
  return true;
}

bool m3u8_next(String_Buffer *temp, M3u8 *m, const char **url) {
  string extinf = STRING("#EXTINF");

  int pos = string_index_of2(m->curr, extinf);
  if(pos != 0)
    return false;

  string_chop_by_delim(&m->curr, '\n');
  string suffix = string_chop_by_delim(&m->curr, '\n');

  printf("suffix: "String_Fmt"\n", String_Arg(suffix));

  *url = tprintf(temp, String_Fmt String_Fmt, String_Arg(m->prefix), String_Arg(suffix) );
  return true;
}

String_Buffer temp = {0};

int main(int argc, char **argv) {

  if(argc < 2) {
    fprintf(stderr, "ERROR: Please provide a link\n");
    fprintf(stderr, "USAGE: %s <link>\n", argv[0]);
    return 1;
  }

  const char *link = argv[1];
  string link_string = string_from_cstr(link);

  if(string_chop_cstr(&link_string, "https://www.twitch.tv/videos/")) {
    return stream_from_id(&temp, link_string);
  } else if(string_chop_cstr(&link_string, "https://www.twitch.tv/")) {    
    return stream_from_channel(&temp, link_string);
  } else {
    fprintf(stderr, "ERROR: This is not a valid link. Cannot extract videoId\n");
    fprintf(stderr, "       The link should look like this: 'https://www.twitch.tv/videos/<id>.'\n");
    fprintf(stderr, "       Or like this: 'https://www.twitch.tv/<channel>'\n");
    return 1;
  }
}
