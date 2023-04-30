#include <stdio.h>

#define HTTP_IMPLEMENTATION
#include "../libs/http.h"

#define HTML_PARSER_IMPLEMENTATION
#include "../libs/html_parser.h"

#define JSON_PARSER_IMPLEMENTATION
#include "../libs/json_parser.h"

#define ARRAY_IMPLEMENTATION
#include "../libs/array.h"

#define PREFIX "var ytInitialPlayerResponse = "

void on_node_content(void *_node, string content, void *arg) {
  if(string_index_of(content, PREFIX) != 0) return;
  *((string *) arg) = content;
}

//Will discard everything allocated in sb
//Returns json stored in sb
bool query_json(const char *url, String_Buffer *sb) {

  sb->len = 0;
  if(!http_get(url, string_buffer_callback, sb, NULL, NULL)) {
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

  sb->len = 0;
  string_buffer_append(sb, json_string.data, json_string.len);

  return true;
}

typedef struct{
  Json_Parse_Type type;
  string name;
  string content;
}Json;

bool on_elem(Json_Parse_Type type, string content, void *arg, void **elem) {
  Json json = {0};
  json.type = type;
  json.content = content;
  *elem = arr_push((Arr *) arg, &json);
  return true;
}

void on_object_elem(void *_object, string key, void *_elem, void *arg) {
  if(_elem == NULL) return;
  Json *elem = (Json *) _elem;
  elem->name = key;
}

void on_array_elem(void *array, void *elem, void *arg) {  
}

String_Buffer sb = {0};
Arr *json_arr = NULL;

int main(int argc, char **argv) {

  if(argc < 2) {
    fprintf(stderr, "ERROR: Please provide the youtube url\n");
    fprintf(stderr, "USAGE: %s <youtube-url\n", argv[0]);
    exit(1);
  }

  if(!query_json(argv[1], &sb)) {
    panic("query_json");
  }
  write_file_len("./rsc/playerData.json", sb.data, sb.len);

  json_arr = arr_init(sizeof(Json));
  if(!json_arr) {
    panic("arr_init");
  }

  Json_Parse_Events events = {0};  
  events.on_elem = on_elem;
  events.on_object_elem = on_object_elem;
  events.on_array_elem = on_array_elem;
  events.arg = json_arr;
  
  if(!json_parse(sb.data, sb.len, &events)) {
    panic("json_parse");
  }

  for(size_t i=0;i<json_arr->count;i++) {
    Json *_elem = arr_get(json_arr, i);
    if(string_eq(_elem->name, STRING("itag")) &&
       string_eq(_elem->content, STRING("140"))) {
      for(size_t j=i;j<json_arr->count;j++) {
	Json *elem = arr_get(json_arr, j);
	printf("(%s) "String_Fmt": '"String_Fmt"'\n",
	       json_parse_type_name(elem->type),
	       String_Arg(elem->name),
	       String_Arg(elem->content));
	if(string_eq(elem->name, STRING("signatureCipher"))) break;
	if(j+2 < json_arr->count) {
	  Json *sub = arr_get(json_arr, j+2);
	  if(string_eq(sub->name, STRING("itag"))) break;
	}
      }
      break;
    }
  }
  
  return 0;
}


