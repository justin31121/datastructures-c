#ifndef YOUTUBE_H_H
#define YOUTUBE_H_H

#ifndef YOUTUBE_DEF
#define YOUTUBE_DEF static inline
#endif //YOUTUBE_DEF

#ifdef YOUTUBE_IMPLEMENTATION

#define HTTP_IMPLEMENTATION
#define JSON_IMPLEMENTATION
#define REGEX_IMPLEMENTATION

// #include "../thirdparty/duktape.c"

#endif //YOUTUBE_IMPLEMENTATION

#include "../thirdparty/duktape.h"
#include "./http.h"
#include "./json.h"
#include "./regex.h"

#define YOUTUBE_HOSTNAME "www.youtube.com"

typedef struct{
  Http http;
  String_Buffer sbs[3];
  Ht *bases;
  duk_context *duk_ctx;
}Youtube_Context;

YOUTUBE_DEF bool youtube_context_init(Youtube_Context *context);
YOUTUBE_DEF void youtube_context_free(Youtube_Context *context);
YOUTUBE_DEF bool youtube_context_start(Youtube_Context *context);
YOUTUBE_DEF void youtube_context_stop(Youtube_Context *context);

const char* get_title(Json videoRenderer);

YOUTUBE_DEF bool youtube_search(Youtube_Context *context, const char *keyword, Json *out);
YOUTUBE_DEF bool youtube_initial_data(Youtube_Context *context, const char *videoId, Json *out);
YOUTUBE_DEF bool youtube_video(Youtube_Context *context, string videoId, Json *out);
YOUTUBE_DEF bool youtube_get_videoId(const char *url, string *videoId);
YOUTUBE_DEF bool youtube_get_audio(Youtube_Context *context, string videoId, char **url, char **name);


#ifdef YOUTUBE_IMPLEMENTATION

YOUTUBE_DEF bool youtube_get_videoId(const char *url, string *videoId) {
    string id = {0};

    string url_string = string_from_cstr(url);

    if(!string_chop_string(&url_string, STRING("https://www.youtube.com/watch?"))) {
	return false;
    }
  
    while(url_string.len) {    
	string pair = string_chop_by_delim(&url_string, '&');
	string value = string_chop_by_delim(&pair, '=');
	if(string_eq(value, STRING("v"))) {
	    id = pair;
	}
    }

    if(!id.len) {
	return false;
    }

    *videoId = id;
    return true;
}

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

YOUTUBE_DEF bool youtube_search(Youtube_Context *context, const char *keyword, Json *out) {
    if(!context) {
	return false;
    }
  
    char buffer[1024];
    if(snprintf(buffer, 1024, "/results?search_query=%s", keyword) >= 1024) {
	return false;
    }

    context->sbs[0].len = 0;
    if(!http_request(&context->http, buffer, "GET", NULL, NULL, string_buffer_callback, &context->sbs[0], NULL, NULL)) {
	return false;
    }

    const char *target = "ytInitialData = ";
    u32 target_len = strlen(target);
    
    string s = string_from(context->sbs[0].data, context->sbs[0].len);
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
    if(!json_parse_len(context->sbs[0].data + off, _pos - off - 1, &json)) {
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

    *out = arr;
  
    return true;
}

YOUTUBE_DEF bool youtube_initial_data(Youtube_Context *context, const char *videoId, Json *out) {
    if(!context || !out || !videoId) {
	return false;
    }

    char route_buffer[1024];
    if(snprintf(route_buffer, 1024, "/watch?v=%s", videoId) >= 1024) {
	return false;
    }

    context->sbs[0].len = 0;
    if(!http_request(&context->http, route_buffer, "GET", NULL, NULL, string_buffer_callback, &context->sbs[0], NULL, NULL)) {
	return false;
    }

    const char *target = "ytInitialData = ";
    u32 target_len = strlen(target);

    string content = string_from(context->sbs[0].data, context->sbs[0].len);
    int start = string_index_of(content, target);
    if(start == -1) {
	return false;
    }

    int end = string_index_of_offset(content, "</script>", start + target_len);
    if(end == -1) {
	return false;
    }

    write_file_len("compact.json", context->sbs[0].data + start + target_len, end - start - target_len - 1);

    Json initialData;
    if(!json_parse_len(context->sbs[0].data + start + target_len, end - start - target_len - 1, &initialData)) {
	return false;
    }

    *out = initialData;
  
    return true;
}

YOUTUBE_DEF bool youtube_video(Youtube_Context *context, string videoId, Json *out) {

    if(!context) {
	return false;
    }

    char route_buffer[1024];
    if(snprintf(route_buffer, 1024, "/watch?v="String_Fmt, String_Arg(videoId)) >= 1024) {
	return false;
    }
  
    String_Buffer *sb = &context->sbs[0];
    sb->len = 0;
    String_Buffer *buffer = &context->sbs[1];
    buffer->len = 0;
    String_Buffer *jsFile = &context->sbs[2];
    jsFile->len = 0;

    if(!http_request(&context->http, route_buffer, "GET", NULL, NULL, string_buffer_callback, sb, NULL, NULL)) {
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
    
	    duk_eval_string(context->duk_ctx, jsFile->data);
	    json_free_all(json_get(format, "url"));
	    json_put_string(format, "url", duk_get_string(context->duk_ctx, -1));
	}
    } else {
	Regex regexs[4] = {0};

	string_buffer_append(sb, "\0", 1);

	// -- base.js
	size_t offset, len;
	if(!regex_compile(&regexs[0], "/s/player/[:alnum:]+/[:alnum_.:]+/[:alnum_:]+/base\\.js")) {
	    panic("can not copmile regex");
	}
	if(!regex_match(&regexs[0], sb->data, &offset, &len)) {
	    panic("regex_match");
	}
	
	//printf("jsFileUrlMatches: %.*s\n", (int) len, sb->data + offset);

	string_buffer_append(buffer, sb->data + offset, len);
	string_buffer_append(buffer, "\0", 1);

	if(context->bases != NULL) {
	    char *base_path = (char *) ht_get(context->bases, buffer->data);
	    if(base_path != NULL) {
		slurp_file2(base_path, string_buffer_callback, jsFile);
	    } else {
		if(!http_request(&context->http, buffer->data, "GET", NULL, NULL, string_buffer_callback, jsFile, NULL, NULL)) {
		    return false;
		}
		string_buffer_append(jsFile, "\0", 1);

		char filename[128];
		if(snprintf(filename, 128, "base%zd.js", context->bases->count) >= 128) {
		    return false;
		}

		write_file_len(filename, jsFile->data, jsFile->len);
		ht_insert(context->bases, buffer->data, filename, strlen(filename));
	    }
	} else {
	    if(!http_request(&context->http, buffer->data, "GET", NULL, NULL, string_buffer_callback, jsFile, NULL, NULL)) {
		return false;
	    }
	    string_buffer_append(jsFile, "\0", 1);
	}
  
	// -- decodeFunction
	if(!regex_compile(&regexs[1], "function[:print:]*\\.split\\(\"\"\\)[:print:]*\\.join\\(\"\"\\)}")) {
	    panic("can not copmile regex");
	}
	if(!regex_match(&regexs[1], jsFile->data, &offset, &len)) {
	    panic("regex_match");
	}

	//printf("decoderFunctionMatches: %.*s\n", (int) len, jsFile->data + offset);

	sb->len = 0;
	string_buffer_append(sb, jsFile->data + offset, len);
	string_buffer_append(sb, "\0", 1);

	// -- varNameMatches
	if(!regex_compile(&regexs[2], "\\.split\\(\"\"\\);[:alnum:]+\\.")) {
	    panic("can not copmile regex");
	}
	if(!regex_match_nth(&regexs[2], sb->data, 0, &offset, &len)) {
	    panic("regex_match");
	}

	len -= 1 + 11;
	offset += 11;
	//printf("varNameMatches: %.*s\n", (int) (len), sb->data + offset);

	buffer->len = 0;
	string_buffer_append(buffer, "var ", 4);
	string_buffer_append(buffer, sb->data + offset, len);
	//const char *expr_suffix = "=\\{.+\\}\\};)[a-zA-Z0-9]+\\.[a-zA-Z0-9]+\\.prototype";
	const char *expr_suffix = "={.+}};[:alnum:]+\\.[:alnum:]+\\.prototype";
	string_buffer_append(buffer, expr_suffix, strlen(expr_suffix));
	string_buffer_append(buffer, "\0", 1);

	// -- varDecalreMatches
	if(!regex_compile(&regexs[3], buffer->data)) {
	    panic("can not copmile regex");
	}
	if(!regex_match_nth(&regexs[3], jsFile->data, 0, &offset, &len)) {
	    panic("regex_match");
	}

	int k = len-1;
	while(k>=0 && jsFile->data[offset+k] != '}') k--;
	len = (size_t) k+2;

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
	    duk_eval_string(context->duk_ctx, jsFile->data);

	    jsFile->len = 0;
	    const char* evaled_string = duk_get_string(context->duk_ctx, -1);
	    string_buffer_append(jsFile, evaled_string, strlen(evaled_string));
	    const char* sig_part = "&sig=";
	    string_buffer_append(jsFile, sig_part, strlen(sig_part));
	    duk_eval_string(context->duk_ctx, buffer->data);
	    const char* second_evaled_string = duk_get_string(context->duk_ctx, -1);
	    string_buffer_append(jsFile, second_evaled_string, strlen(second_evaled_string));
	    string_buffer_append(jsFile, "\0", 1);
	    json_put_string(format, "url", jsFile->data);
	}

	////////////////////////////////////

	for(u32 i=0;i<(sizeof(regexs)/sizeof(regexs[0]));i++) {
	    regex_free(&regexs[i]);
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

#define YOUTUBE_TRIES 3

YOUTUBE_DEF bool youtube_get_audio(Youtube_Context *context, string videoId, char **url, char **name) {

  HttpHeader header;
  bool url_is_valid = false;

  if(name) *name = NULL;

  for(int i=0;i<3;i++) {
    Json out;
    if(!youtube_video(context, videoId, &out)) {
      return false;
    }

    if(name && !(*name)) {
      if(json_has(out, "videoDetails")) {
	Json videoDetails = json_get(out, "videoDetails");
	if(json_has(videoDetails, "title")) {
	  *name = json_get_string(videoDetails, "title");
	  json_put_null(videoDetails, "title");
	}	
      }
    }

    Json formats = json_get(out, "formats");
    for(s32 j=0;j<json_size(formats);j++) {
      Json format = json_opt(formats, j);
      int itag = json_get_int(format, "itag");
      if(itag == 140) {      
	*url = json_get_string(format, "url");

	if(!http_head((const char *) *url, &header, NULL)) {
	  break;
	}
	s32 responseCode = http_header_response_code(&header);
	url_is_valid = responseCode == 200;

	if(url_is_valid) {
	  json_put_null(format, "url");
	}
	break;
      }
    }
    
    json_free_all(out);
    if(url_is_valid) break;
  }

  return url_is_valid;
}

YOUTUBE_DEF bool youtube_context_init(Youtube_Context *context) {  
  for(size_t i=0;i<3;i++) context->sbs[i] = (String_Buffer) {0};
  context->bases = NULL;
  context->duk_ctx = duk_create_heap_default();
  return true;
}

YOUTUBE_DEF bool youtube_context_start(Youtube_Context *context) {
  return http_init2(&context->http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true);
}

YOUTUBE_DEF void youtube_context_stop(Youtube_Context *context) {
  http_free(&context->http);
}

YOUTUBE_DEF void youtube_context_free(Youtube_Context *context) {
  for(size_t i=0;i<3;i++) string_buffer_free(&context->sbs[i]);
  if(context->bases) ht_free(context->bases);
  duk_destroy_heap(context->duk_ctx);
}


#endif //YOUTUBE_IMPLEMENTATION

#endif //YOUTUBE_H_H
