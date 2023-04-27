#include <stdio.h>

#define HTTP_IMPLEMENTATION
#include "../libs/http.h"

#define HTML_PARSER_IMPLEMENTATION
#include "../libs/html_parser.h"

#define JSON_PARSER_IMPLEMENTATION
#include "../libs/json_parser.h"

void on_node_content(void *_node, string content, void *arg) {
  if(string_index_of(content, "var ytInitialData") != 0) return;
  *((string *) arg) = content;
}

//Will discard everything allocated in sb
//Returns json stored in sb
bool query_json(String_Buffer *sb) {

  sb->len = 0;
  if(!http_get("https://www.youtube.com/", string_buffer_callback, sb, NULL, NULL)) {
    return false;
  }

  string json_string;
  
  Html_Parse_Events events = {0};
  events.on_node_content = on_node_content;
  events.arg = &json_string;
  
  if(!html_parse(sb->data, sb->len, &events)) {
    return false;
  }

  string_chop_left(&json_string, 20); // 'var ytInitialData = '
  string_chop_right(&json_string, 1); // ';'

  sb->len = 0;
  string_buffer_append(sb, json_string.data, json_string.len);

  return true;
}

String_Buffer sb = {0};

bool on_elem(Json_Parse_Type type, string content, void *arg, void **elem) {
    printf("got elem: "String_Fmt" (%s)\n",
	   String_Arg(content),
	   json_parse_type_name(type));
    if(type == JSON_PARSE_TYPE_INT) {
	int *d = malloc(sizeof(int));
	string_chop_int(&content, d);
	*elem = d;
    }
    return true;
}

void on_object_elem(void *object, string key, void *elem, void *arg) {
    if(string_eq(key, STRING("code"))) {
	printf("code: %d\n", *(int *) elem);
    }
}

void on_array_elem(void *array, void *elem, void *arg) {
}

int main() {
  /*
  if(!query_json(&sb)) {
    panic("query_json");
  }
  */

  const char *payload = "{ \"code\": 200  }";
  
  string_buffer_append(&sb, payload, strlen(payload));

  Json_Parse_Events events = {0};  
  events.on_elem = on_elem;
  events.on_object_elem = on_object_elem;
  events.on_array_elem = on_array_elem;
  
  if(!json_parse(sb.data, sb.len, &events)) {
    panic("json_parse");
  }

  printf("ok\n");
  
  return 0;
}


