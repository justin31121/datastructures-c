#ifndef YOUTUBE_H_H
#define YOUTUBE_H_H

#ifdef YOUTUBE_IMPLEMENTATION

#define HTTP_IMPLEMENTATION
#define JSON_IMPLEMENTATION

// #include "../thirdparty/duktape.c"

#endif //YOUTUBE_IMPLEMENTATION

#include "../thirdparty/duktape.h"
#include "../libs/http.h"
#include "../libs/json.h"

#include <regex.h>

bool match_regexp(regex_t *regex, const char *cstr, const char *regexp_cstr, int match_index, int *offset, int *len);
const char* get_title(Json videoRenderer);

bool youtube_search(Http *http, const char *keyword, Json *out);
bool youtube_initial_data(Http *http, String_Buffer *sb, const char *videoId, Json *out);
bool youtube_video(Http *http, String_Buffer sbs[3], Ht *bases, duk_context *duk_ctx, const char *videoId, Json *out);

#ifdef YOUTUBE_IMPLEMENTATION

const char* get_title(Json videoRenderer) {
  if(json_has(videoRenderer, "title")) {
    Json title = json_get(videoRenderer, "title");
    if(json_has(title, "runs")) {
      Json runs = json_get(title, "runs");
      if(json_size(runs) > 0) {
	Json run = json_opt(runs, 0);
	return json_get_string(run, "text");
      }
    }
  }

  return NULL;
}

bool match_regexp(regex_t *regex, const char *cstr, const char *regexp_cstr, int match_index, int *offset, int *len) {
  if(!regex) {
    return false;
  }
  
  regmatch_t regmatch[4] = {0};
  if(match_index > 3) {
    warn("increase number of regmatches manually");
    return false;
  }
  
  if (regcomp(regex, regexp_cstr, REG_EXTENDED)) {
    warn("can not compile regexp");
    return false;
  }
  
  int result;
  bool ret = false;
  if(!(result =regexec(regex, cstr, 2, regmatch, 0))) {
    if(regmatch[match_index].rm_so == -1 || regmatch[match_index].rm_eo == -1) {
      warn("match_index out of bounce");
      ret = false;
    }
    if(offset) *offset = regmatch[match_index].rm_so;
    if(len) {
      *len = (int)(regmatch[match_index].rm_eo - regmatch[match_index].rm_so);
    }
    ret = true;
  } else if(result == REG_NOMATCH) {
    ret = false;
  } else {
    panic("Error in regexp execution");
  }
  return ret;
}

bool youtube_search(Http *http, const char *keyword, Json *out) {
  if(!http) {
    return false;
  }
  
  char buffer[10240];
  if(snprintf(buffer, 10240, "/results?search_query=%s",
	      keyword) >= 10240) {
    return false;
  }

  String_Buffer sb = {0};
  if(!http_request(http, buffer, "GET", NULL, NULL, string_buffer_callback, &sb, NULL, NULL)) {
    return false;
  }

  const char *target = "ytInitialData = ";
  u32 target_len = strlen(target);
    
  string s = string_from(sb.data, sb.len);
  s32 pos = string_index_of(s, target);
  if(pos < 0) {
    return false;
  }

  s32 off = pos + target_len;
  const char *target2 = "</script>";

  s32 _pos = string_index_of_offset(s, target2, off);
  if(_pos < 0) {
    return false;
  }

  Json json;
  if(!json_parse_len(sb.data + off, _pos - off - 1, &json)) {
    return false;
  }

  Json arr = json_init_array();
  Json _contents = json_get(json_get(json_get(json_get(json_get(json, "contents"), "twoColumnSearchResultsRenderer"), "primaryContents"), "sectionListRenderer"), "contents");

  //TODO: memory management of json.h making the copying hard
  for(s32 i=0;i<json_size(_contents);i++) {
    Json _content = json_opt(_contents, i);
    
    if(!json_has(_content, "itemSectionRenderer")) {
      continue;
    }
    Json itemSectionRenderer = json_get(_content, "itemSectionRenderer");

    if(!json_has(itemSectionRenderer, "contents")) {
      continue;
    }

    Json contents = json_get(itemSectionRenderer, "contents");
    for(int j=0;j<json_size(contents);j++) {
      Json result = json_opt(contents, j);
      if(json_has(result, "videoRenderer")) {
	Json videoRenderer = json_get(result, "videoRenderer");
        json_push_object(arr, &videoRenderer);

	//thats awkward
	json_put_null(result, "videoRenderer");
      }

      if(!json_has(result, "shelfRenderer")) {
	continue;
      }
      Json shelfRenderer = json_get(result, "shelfRenderer");

      if(!json_has(shelfRenderer, "content")) {
	continue;
      }
      Json content = json_get(shelfRenderer, "content");

      if(!json_has(content, "verticalListRenderer")) {
	continue;
      }
      Json verticalListRenderer = json_get(content, "verticalListRenderer");

      if(!json_has(verticalListRenderer, "items")) {
	continue;
      }
      Json items = json_get(verticalListRenderer, "items");

      for(s32 k=0;k<json_size(items);k++) {
	Json shelf = json_opt(items, k);
	if(!json_has(shelf, "videoRenderer")) {
	  continue;
	}
	Json videoRenderer = json_get(shelf, "videoRenderer");
	json_push_object(arr, &videoRenderer);

	//thats awkward
        json_put_null(shelf, "videoRenderer");
      }
    }
  }

  json_free_all(json);
  string_buffer_free(&sb);

  *out = arr;
  
  return true;
}

bool youtube_initial_data(Http *http, String_Buffer *sb, const char *videoId, Json *out) {
  if(!http || !sb || !out || !videoId) {
    return false;
  }

  char route_buffer[1024];
  if(snprintf(route_buffer, 1024, "/watch?v=%s", videoId) >= 1024) {
    return false;
  }

  if(!http_request(http, route_buffer, "GET", NULL, NULL, string_buffer_callback, sb, NULL, NULL)) {
    return false;
  }

  const char *target = "ytInitialData = ";
  u32 target_len = strlen(target);

  string content = string_from(sb->data, sb->len);
  int start = string_index_of(content, target);
  if(start == -1) {
    return false;
  }

  int end = string_index_of_offset(content, "</script>", start + target_len);
  if(end == -1) {
    return false;
  }

  write_file_len("compact.json", sb->data + start + target_len, end - start - target_len - 1);

  Json initialData;
  if(!json_parse_len(sb->data + start + target_len, end - start - target_len - 1, &initialData)) {
    return false;
  }

  *out = initialData;
  
  return true;
}

bool youtube_video(Http *http, String_Buffer sbs[3], Ht *bases, duk_context *duk_ctx, const char *videoId, Json *out) {

  if(!http) {
    return false;
  }

  char route_buffer[1024];
  if(snprintf(route_buffer, 1024, "/watch?v=%s", videoId) >= 1024) {
    return false;
  }
  
  String_Buffer *sb = &sbs[0];
  sb->len = 0;
  String_Buffer *buffer = &sbs[1];
  buffer->len = 0;
  String_Buffer *jsFile = &sbs[2];
  jsFile->len = 0;

  if(!http_request(http, route_buffer, "GET", NULL, NULL, string_buffer_callback, sb, NULL, NULL)) {
    return false;
  }

  string content = string_from(sb->data, sb->len);
  int start = string_index_of(content, "ytInitialPlayerResponse");
  if(start == -1) {
    return false;
  }

  int end = string_index_of_offset(content, "</script>", start);
  if(end == -1) {
    return false;
  }

  Json initialPlayerResponse;
  if(!json_parse_len(sb->data + start + 26, end - start - 27, &initialPlayerResponse)) {
    return false;
  }

  Json streamingData = {0};
  if(json_has(initialPlayerResponse, "streamingData")) {
    streamingData = json_get(initialPlayerResponse, "streamingData"); 
  } else {
    json_free_all(initialPlayerResponse);
    return false;
  }

  Json video = json_init_object();
  Json formats = json_init_array();
  if(streamingData.type && json_has(streamingData, "formats")) {
    Json _formats = json_get(streamingData, "formats");
    for(int i=0;i<json_size(_formats);i++) {
      Json _format = json_opt_object(_formats, i);
      json_push_object(formats, &_format);
    }
  }  
  if(streamingData.type && json_has(streamingData, "adaptiveFormats")) {
    Json adaptiveFormats = json_get(streamingData, "adaptiveFormats");
    for(int i=0;i<json_size(adaptiveFormats);i++) {
      Json adaptiveFormat = json_opt_object(adaptiveFormats, i);
      json_push_object(formats, &adaptiveFormat);
    }
  }
  json_put_array(video, "formats", &formats);

  Json videoDetails = json_get(initialPlayerResponse, "videoDetails");
  if(json_has(videoDetails, "isLive") && json_get_bool(videoDetails, "isLive")) {
    const char *m3u8Link = json_get_string(streamingData, "hlsManifestUrl");
    json_put_string(video, "hlsManifestUrl", m3u8Link);
  }
  if(streamingData.type && json_has(streamingData, "dashManifestUrl")) {
    const char *dashLink = json_get_string(streamingData, "dashManifestUrl");
    json_put_string(video, "dashManifestUrl", dashLink);
  }
  json_put_object(video, "videoDetails", &videoDetails);

  bool need_decryption = false;
  for(int i=0;i<json_size(formats);i++) {
    Json format = json_opt_object(formats, i);
    need_decryption |= json_has(format, "signatureCipher");
  }
  
  if(!need_decryption) {
    //TODO: use encudeURIComponent ?
    for(int i=0;i<json_size(formats);i++) {
      Json format = json_opt(formats, i);
	
      const char *url = json_get_string(format, "url");
      const char *prefix_encode = "decodeURIComponent(\"";
      string_buffer_append(jsFile, prefix_encode, strlen(prefix_encode));
      string_buffer_append(jsFile, url, strlen(url));
      string_buffer_append(jsFile, "\")\0", 3);
    
      duk_eval_string(duk_ctx, jsFile->data);
      json_free_all(json_get(format, "url"));
      json_put_string(format, "url", duk_get_string(duk_ctx, -1));
    }
  } else {
    regex_t regexs[4] = {0};

    string_buffer_append(sb, "\0", 1);

    // -- base.js
    int offset, len;
    if(!match_regexp(&regexs[0], sb->data, "\\/s\\/player\\/[A-Za-z0-9]+\\/[A-Za-z0-9_.]+\\/[A-Za-z0-9_]+\\/base\\.js", 0, &offset, &len)) {
      panic("Can not find base.js");
    }

    string_buffer_append(buffer, sb->data + offset, len);
    string_buffer_append(buffer, "\0", 1);

    if(bases != NULL) {
      char *base_path = (char *) ht_get(bases, buffer->data);
      if(base_path != NULL) {
	slurp_file2(base_path, string_buffer_callback, jsFile);
      } else {
	if(!http_request(http, buffer->data, "GET", NULL, NULL, string_buffer_callback, jsFile, NULL, NULL)) {
	  return false;
	}
	string_buffer_append(jsFile, "\0", 1);

	char filename[128];
	if(snprintf(filename, 128, "base%lu.js", bases->count) >= 128) {
	  return false;
	}

	write_file_len(filename, jsFile->data, jsFile->len);
	ht_insert(bases, buffer->data, filename, strlen(filename));
      }
    } else {
      	if(!http_request(http, buffer->data, "GET", NULL, NULL, string_buffer_callback, jsFile, NULL, NULL)) {
	  return false;
	}
	string_buffer_append(jsFile, "\0", 1);
    }
  
    // -- decodeFunction
    const char * decodeFunctionExpr= "function[[:print:]]*\\.split\\(\\\"\\\"\\)[[:print:]]*\\.join\\(\\\"\\\"\\)\\}";
    if(!match_regexp(&regexs[1], jsFile->data, decodeFunctionExpr, 0, &offset, &len)) {
      panic("Can not find decodeFunction");
    }

    sb->len = 0;
    string_buffer_append(sb, jsFile->data + offset, len);
    string_buffer_append(sb, "\0", 1);

    // -- varNameMatches
    if(!match_regexp(&regexs[2], sb->data, "\\.split\\(\\\"\\\"\\);([a-zA-Z0-9]+)\\.", 1, &offset, &len)) {
      panic("Can not find varNameMatches");
    }

    buffer->len = 0;
    string_buffer_append(buffer, "(var ", 5);
    string_buffer_append(buffer, sb->data + offset, len);
    const char *expr_suffix = "=\\{.+\\}\\};)[a-zA-Z0-9]+\\.[a-zA-Z0-9]+\\.prototype";
    string_buffer_append(buffer, expr_suffix, strlen(expr_suffix));
    string_buffer_append(buffer, "\0", 1);

    // -- varDecalreMatches
    if(!match_regexp(&regexs[3], jsFile->data, buffer->data, 1, &offset, &len)) {
      panic("Can not find varDeclareMatches");
    }

    /////////////////////////////////////
    for(int j=0;j<json_size(formats);j++) {
      Json format = json_opt(formats, j);
	
      const char *_signatureCipher = json_get_string(format, "signatureCipher");
      string signatureCipher = string_from_cstr(_signatureCipher);
      string_chop_by_delim(&signatureCipher, '=');
      string _s = string_chop_by_delim(&signatureCipher, '=');
      string __s = string_from(_s.data, _s.len - 8);
      string url = {0};

      while(signatureCipher.len) {
	string param = string_chop_by_delim(&signatureCipher, '=');
	if(param.len < 6) continue;
	const char * https_cstr = "https:";
	bool ccontinue = false;
	for(int i=0;i<6;i++) {
	  if(https_cstr[i] != param.data[i]) {
	    ccontinue = true;
	    break;
	  }
	}
	if(ccontinue) continue;
	url = param;
      }

      buffer->len = 0;
      const char* use_strict = "\"use-strict\";";
      string_buffer_append(buffer, use_strict, strlen(use_strict));
      string foo_string = string_from(jsFile->data + offset, len);
      while(foo_string.len) {
	string foo_string_part = string_chop_by_delim(&foo_string, '\n');
	string_buffer_append(buffer, foo_string_part.data, foo_string_part.len);
      }
      const char *encodeURIComponentCstr = "encodeURIComponent((";
      string_buffer_append(buffer, encodeURIComponentCstr, strlen(encodeURIComponentCstr));
      string_buffer_append(buffer, sb->data, sb->len - 1); //decodeFunction
      string_buffer_append(buffer, ")(", 2);
      const char *signature = "decodeURIComponent(\"";
      string_buffer_append(buffer, signature, strlen(signature));
      string_buffer_append(buffer, __s.data, __s.len);
      string_buffer_append(buffer, "\")));\0", 6);

      jsFile->len = 0;
      const char *prefix_encode = "decodeURIComponent(\"";
      string_buffer_append(jsFile, prefix_encode, strlen(prefix_encode));
      string_buffer_append(jsFile, url.data, url.len);
      string_buffer_append(jsFile, "\")\0", 3);
      duk_eval_string(duk_ctx, jsFile->data);

      jsFile->len = 0;
      const char* evaled_string = duk_get_string(duk_ctx, -1);
      string_buffer_append(jsFile, evaled_string, strlen(evaled_string));
      const char* sig_part = "&sig=";
      string_buffer_append(jsFile, sig_part, strlen(sig_part));
      duk_eval_string(duk_ctx, buffer->data);
      const char* second_evaled_string = duk_get_string(duk_ctx, -1);
      string_buffer_append(jsFile, second_evaled_string, strlen(second_evaled_string));
      string_buffer_append(jsFile, "\0", 1);
      json_put_string(format, "url", jsFile->data);
    }

    ////////////////////////////////////

    for(u32 i=0;i<(sizeof(regexs)/sizeof(regexs[0]));i++) {
      regfree(&regexs[i]);
    }
  }
  
  *out = video;

  //FREE ALL, EXCEPT used FORMATS
  Ht_Entry *entry;
  int last = -1;
  while(streamingData.type && ht_next(streamingData.objVal, &last, &entry)) {
    if( (strcmp("formats", entry->key) == 0) ||
	(strcmp("adaptiveFormats", entry->key) == 0)
	) {
      json_free(*(Json *) entry->value);
      continue;
    }
    json_free_all(*(Json *) entry->value);
  }

  
  json_put_null(initialPlayerResponse, "videoDetails");
  if(streamingData.type) {
    ht_free(streamingData.objVal);
  }
  json_put_null(initialPlayerResponse, "streamingData");
  json_free_all(initialPlayerResponse);
  
  return true;
}

#endif //YOUTUBE_IMPLEMENTATION

#endif //YOUTUBE_H_H
