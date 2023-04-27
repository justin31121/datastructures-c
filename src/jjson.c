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

bool on_object(void *arg, void **object) {
  return true;
}

void on_object_elem(void *object, string key, void *elem, void *arg) {
  
}

bool on_array(void *arg, void **array) {
  return true;
}

void on_array_elem(void *array, void *elem) {
  
}

int main() {
  /*
  if(!query_json(&sb)) {
    panic("query_json");
  }
  */

  const char *payload = "true";
  string_buffer_append(&sb, payload, strlen(payload));

  Json_Parse_Events events = {0};  
  
  if(!json_parse(sb.data, sb.len, &events)) {
    panic("json_parse");
  }

  printf("ok\n");
  
  return 0;
}
