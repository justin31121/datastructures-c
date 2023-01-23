#include <stdio.h>
#include <stdlib.h>

#define HTTP_IMPLEMENTATION
#include "../libs/http.h"

#define JSON_IMPLEMENTATION
#include "../libs/json.h"

String_Buffer sb = {0};

int main1() {
  if(!http_get("http://127.0.0.1/index.html", _fwrite, stdout)) {
    panic("request failed");
  }  
  return 0;
}

int main() { 
  if(!http_get("https://www.youtube.com", string_buffer_callback, &sb)) {
    panic("request failed");
  }

  if(sb.len == 0) {
    panic("No bytes in the string buffer");
  }
  
  //PARSE JSON
  string response = string_from(sb.data, sb.len);
  int pos = string_index_of(response, "ytInitialData = ");
  if(pos < 0) {
    panic("Can not find ytInitialData");
  }

  int _pos = pos + strlen("ytInitialData = ");
  if(_pos < 0) {
    panic("Can not find ytInitialData");
  }  

  int j = string_index_of_offset(response, ";</script>", pos);
  if(j < 0) {
    panic("Can not find ytInitialData");
  }  

  string jsonString = string_trim(string_substring(response, _pos, j));

  Json json;
  if(!json_parse_len(jsonString.data, jsonString.len, &json)) {
    write_file_len("initialData.json", jsonString.data, jsonString.len);
    panic("Can not parse json");
  }

  //EXTRACT JSON
  const Json contents = json_get_array(json_get_object(json_get_object(json_get_object(json_opt_object(json_get_array(json_get_object(json_get_object(json, "contents"),"twoColumnBrowseResultsRenderer"),"tabs"),0),"tabRenderer"),"content"),"richGridRenderer"), "contents");

  bool written = false;
  
  for(int i=0;i<json_size(contents);i++) {
    Json item = json_opt_object(contents, i);
    if(!json_has(item, "richItemRenderer")) continue;
    Json richItemRenderer = json_get_object(item, "richItemRenderer");
    if(!json_has(richItemRenderer, "content")) continue;
    Json content = json_get_object(richItemRenderer, "content");
    if(!json_has(content, "videoRenderer")) continue;
    Json videoRenderer = json_get_object(content, "videoRenderer");

    //TITLE
    if(!json_has(videoRenderer, "title")) continue;
    Json title = json_get_object(videoRenderer, "title");
    if(!json_has(title, "runs")) continue;
    Json runs = json_get_array(title, "runs");
    if(json_size(runs) == 0 ) continue;
    printf("Title: %s\n", json_get_string(json_opt_object(runs, 0), "text"));

    //THUMBNAIL
    if(!json_has(videoRenderer, "thumbnail")) continue;
    Json thumbnail = json_get_object(videoRenderer, "thumbnail");
    if(!json_has(thumbnail, "thumbnails")) continue;
    Json thumbnails = json_get_array(thumbnail, "thumbnails");
    if(json_size(thumbnails) == 0) continue;
    Json pic = json_opt_object(thumbnails, json_size(thumbnails) - 1);
    if(!json_has(pic, "url")) continue;
    char * pic_url = json_get_string(pic, "url");
    printf("%s\n", pic_url);

    if(!written) {
      printf("\tTrying to safe %s\n", pic_url);
      written = true;

      sb.len = 0;
      if(!http_get(pic_url, string_buffer_callback, &sb)) {
	panic("failed to download pic");
      }

      printf("\tSuccesfully downloaded pic\n");
      write_file_len("test.jpg", sb.data, sb.len);
      break;
    }
  }

  json_free_all(json);
  return 0;
}
