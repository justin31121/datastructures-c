#ifndef YOUTUBE_H_H
#define YOUTUBE_H_H

#ifndef YOUTUBE_DEF
#define YOUTUBE_DEF static inline
#endif //YOUTUBE_DEF

#ifdef YOUTUBE_IMPLEMENTATION

#define HTTP_IMPLEMENTATION
#define JSON_IMPLEMENTATION
#define REGEX_IMPLEMENTATION
#define HTML_PARSER_IMPLEMENTATION
#define JSON_PARSER_IMPLEMENTATION

// #include "../thirdparty/duktape.c"

#endif //YOUTUBE_IMPLEMENTATION

#include "../thirdparty/duktape.h"
#include "./http.h"
#include "./json.h"
#include "./regex.h"
#include "./html_parser.h"
#include "./json_parser.h"

#define YOUTUBE_HOSTNAME "www.youtube.com"

typedef struct{
    string prev;
    Arr *videoIds;
    int state;
}Youtube_Results;

YOUTUBE_DEF bool youtube_results_init(string term, Http *http, String_Buffer *sb, Youtube_Results *results);
YOUTUBE_DEF void youtube_results_dump(Youtube_Results *results);
YOUTUBE_DEF void youtube_results_free(Youtube_Results *results);

typedef struct{
    string dash_source;
    string m3u8_source;
    string prev;
    string prev_prev;
    int state;
    Arr *strings;
}Youtube_Info;

YOUTUBE_DEF bool youtube_get_response(string videoId, Http * http, String_Buffer *sb, string *response);

YOUTUBE_DEF bool youtube_info_init(string response_string, Youtube_Info *info);
YOUTUBE_DEF void youtube_info_dump(Youtube_Info *info);
YOUTUBE_DEF bool youtube_info_find_stream(Youtube_Info *info, string tag, string *signature, bool *is_signature);
YOUTUBE_DEF bool youtube_info_find_audio(Youtube_Info *info, string *signature, bool *is_signature);
YOUTUBE_DEF bool youtube_info_find_video(Youtube_Info *info, string *signature, bool *is_signature);
YOUTUBE_DEF void youtube_info_free(Youtube_Info *info);

typedef struct{
    duk_context* duk_ctx;
    string decodeFunction;
    string prefix;
}Youtube_Decoder;

YOUTUBE_DEF bool youtube_decoder_init(string response_string, Http *http, String_Buffer *sb, Youtube_Decoder *decoder);
YOUTUBE_DEF bool youtube_decoder_decode(Youtube_Decoder *decoder, String_Buffer *sb, string signature, const char **url);
YOUTUBE_DEF void youtube_decoder_free(Youtube_Decoder *decoder);

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
YOUTUBE_DEF bool youtube_get_audio2(string videoId, String_Buffer *sb, string *url, string *name);

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
    size_t target_len = strlen(target);
    
    string s = string_from(context->sbs[0].data, context->sbs[0].len);
    s32 pos = string_index_of(s, target);
    if(pos < 0) {
	return false;
    }

    s32 off = pos + (s32) target_len;
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
    size_t target_len = strlen(target);

    string content = string_from(context->sbs[0].data, context->sbs[0].len);
    int start = string_index_of(content, target);
    if(start == -1) {
	return false;
    }

    int end = string_index_of_offset(content, "</script>", start + target_len);
    if(end == -1) {
	return false;
    }

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
	    //panic("regex_match");
	    return false;
	}
	
	//printf("jsFileUrlMatches: %.*s\n", (int) len, sb->data + offset);

	string_buffer_append(buffer, sb->data + offset, len);
	string_buffer_append(buffer, "\0", 1);

	/*
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
	*/
	if(!http_request(&context->http, buffer->data, "GET", NULL, NULL, string_buffer_callback, jsFile, NULL, NULL)) {
	  return false;
	}
	string_buffer_append(jsFile, "\0", 1);
	/*
	  }
	*/
  
	// -- decodeFunction
	if(!regex_compile(&regexs[1], "function[:print:]*\\.split\\(\"\"\\)[:print:]*\\.join\\(\"\"\\)}")) {
	    panic("can not copmile regex");
	}
	if(!regex_match(&regexs[1], jsFile->data, &offset, &len)) {
	    //panic("regex_match");
	    return false;
	}

	//printf("decoderFunctionMatches: %.*s\n", (int) len, jsFile->data + offset);

	sb->len = 0;
	string_buffer_append(sb, jsFile->data + offset, len);
	string_buffer_append(sb, "\0", 1);

	// -- varNameMatches
	if(!regex_compile(&regexs[2], "\\.split\\(\"\"\\);[:alnum:]+\\.")) {
	    panic("can not copmile regex");
	}
	if(!regex_match(&regexs[2], sb->data, &offset, &len)) {
	    //panic("regex_match");
	    return false;
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
	if(!regex_match(&regexs[3], jsFile->data, &offset, &len)) {
	    //panic("regex_match");
	    return false;
	}

	int k = (int) len-1;
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
    while(streamingData.type && ht_next(streamingData.as.objVal, &last, &entry)) {
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
	ht_free(streamingData.as.objVal);
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

YOUTUBE_DEF bool youtube_get_audio2(string videoId, String_Buffer *sb, string *url, string *name) {
    Http http;
    if(!http_init2(&http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
	panic("http_init2");
    }

    string_buffer_reserve(sb, 4 * 1024 * 1024);
    string response;
    if(!youtube_get_response(videoId, &http, sb, &response)) {
	panic("youtube_get_response");
    }

    Youtube_Info info;
    if(!youtube_info_init(response, &info)) {
	panic("youtube_info_init");
    }
    youtube_info_dump(&info);

    string signature;
    bool is_signature;
    if(!youtube_info_find_audio(&info, &signature, &is_signature)) {
	panic("youtube_info_find_stream");
    }

    Youtube_Decoder decoder;
    if(!youtube_decoder_init(response, &http, sb, &decoder)) {
	panic("youtube_decoder_init");
    }
    printf(String_Fmt"\n", String_Arg(signature)); 
    printf("here\n");fflush(stdout);

    String_Buffer temp = {0};
    const char *stream_url;
    if(!youtube_decoder_decode(&decoder, &temp, signature, &stream_url)) {
	panic("youtube_decoder_decode");
    }
    
    return false;
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

////////////////////////////////////////////////////////////////////

string YOUTUBE_INFO_FRAME = STRING_STATIC("YOUTUBE_INFO_FRAME");

YOUTUBE_DEF bool youtube_on_elem_info_init(Json_Parse_Type type, string content, void *arg, void **elem) {
    (void) type;
    (void) elem;
    Youtube_Info *foo = (Youtube_Info *) arg;

    //printf(String_Fmt"\n", String_Arg(content));
    
    if(foo->state == 0) {
	if( (string_index_of(content, "video/mp4") >= 0)  ||
	    (string_index_of(content, "video/webm") >= 0) ||
	    (string_index_of(content, "audio/mp4") >= 0)  ||
	    (string_index_of(content, "audio/webm") >= 0) ) {
	    if(string_index_of(foo->prev, "https://") == 0) {
		arr_push(foo->strings, &YOUTUBE_INFO_FRAME);
		arr_push(foo->strings, &foo->prev_prev);
		arr_push(foo->strings, &content);
		arr_push(foo->strings, &foo->prev);
	    } else {
		foo->state = 1;
		arr_push(foo->strings, &YOUTUBE_INFO_FRAME);
		arr_push(foo->strings, &foo->prev);
		arr_push(foo->strings, &content);
	    }      
	}
    } else if(foo->state == 1) {
	if(string_index_of(content, "s=") == 0) {
	    string sig = STRING("sig");
	    arr_push(foo->strings, &sig);
	    arr_push(foo->strings, &content);
	    foo->state = 0;
	}
    }

    foo->prev_prev = foo->prev;
    foo->prev = content;
    return true;
}

YOUTUBE_DEF void youtube_on_object_elem_info_init(void *object, string key, void *elem, void *arg) {
    (void) object;
    (void) key;
    (void) elem;
    Youtube_Info *foo = (Youtube_Info *) arg;

    if(string_eq(key, STRING("hlsManifestUrl"))) {
	foo->m3u8_source = foo->prev;
    } else if(string_eq(key, STRING("dashManifestUrl"))) {
	foo->dash_source = foo->prev;
    }

    if(foo->state == 1) {
	arr_push(foo->strings, &key);
	arr_push(foo->strings, &foo->prev);
    }
}

typedef struct{
    string content;
    const char *prefix;
}Youtube_JavaScript_Content;

YOUTUBE_DEF void youtube_on_node_content_query_json(void *_node, string content, void *arg) {
    (void) _node;
    Youtube_JavaScript_Content *js_content = (Youtube_JavaScript_Content *) arg;
    if(string_index_of(content, js_content->prefix) != 0) return;
    js_content->content = content;
}

YOUTUBE_DEF bool youtube_get_response(string videoId, Http * http, String_Buffer *sb, string *response) {
    size_t sb_len = sb->len;
    const char *route = tprintf(sb, "/watch?v="String_Fmt, String_Arg(videoId));

    sb->len = sb_len;
    if(!http_request(http, route, "GET", NULL, NULL, string_buffer_callback, sb, NULL, NULL)) {
	return false;
    }

    *response = string_from(sb->data + sb_len, sb->len - sb_len);
    return true;
}

YOUTUBE_DEF bool youtube_info_init(string response_string, Youtube_Info *info) {
    Youtube_JavaScript_Content js_content;
    js_content.content = (string) {0};
    js_content.prefix = "var ytInitialPlayerResponse = ";
  
    Html_Parse_Events events = {0};
    events.on_node_content = youtube_on_node_content_query_json;
    events.arg = &js_content;
  
    if(!html_parse(response_string.data, response_string.len, &events)) {
	return false;
    }
    string json_string = js_content.content;
    string_chop_left(&json_string, strlen(js_content.prefix));
    string_chop_right(&json_string, 1);

    info->state = 0;
    info->strings = arr_init2(sizeof(string), 1024);
    info->dash_source = (string) {0};
    info->m3u8_source = (string) {0};

    Json_Parse_Events json_events = {0};
    json_events.on_elem = youtube_on_elem_info_init;
    json_events.on_object_elem = youtube_on_object_elem_info_init;
    json_events.arg = info;

    if(!json_parse2(json_string.data, json_string.len, &json_events)) {
	return false;
    }
    
    return true;
}

YOUTUBE_DEF void youtube_info_free(Youtube_Info *info) {
    arr_free(info->strings);
}

YOUTUBE_DEF void youtube_info_dump(Youtube_Info *info) {
    const char *display[] = {"width", "height", "fps", "audioSampleRate"};

    size_t i=1;
    while(i<info->strings->count) {
	string tag = *(string *) arr_get(info->strings, i++);
	string type = *(string *) arr_get(info->strings, i++);

	printf(String_Fmt" - "String_Fmt"\n", String_Arg(tag), String_Arg(type));

	string next = *(string *) arr_get(info->strings, i++);
	while(!string_eq(next, YOUTUBE_INFO_FRAME)) {
	    string value = *(string *) arr_get(info->strings, i++);

	    bool hide = true;
	    size_t display_len = sizeof(display)/sizeof(display[0]);
	    for(size_t j=0;j<display_len;j++) {
		if(string_eq_cstr(next, display[j])) {
		    hide = false; break;
		}
	    }
	    if(!hide) {
		printf("\t"String_Fmt":'"String_Fmt"'\n", String_Arg(next), String_Arg(value));
	    }
	
	    if(i == info->strings->count) break;
	    next = *(string *) arr_get(info->strings, i++);
	}
    }
}

YOUTUBE_DEF bool youtube_info_find_stream(Youtube_Info *info, string tag, string *signature, bool *is_signature) {
    *signature = (string) {0};

    size_t i=1;    
    while(i<info->strings->count) {
	string _tag = *(string *) arr_get(info->strings, i++);
	i++; //string type = *(string *) arr_get(info->strings, i++);

	string _signature = {0};
	string next = *(string *) arr_get(info->strings, i++);
	while(!string_eq(next, YOUTUBE_INFO_FRAME)) {
	    string value = *(string *) arr_get(info->strings, i++);
	    if(string_eq_cstr(next, "sig")) _signature = value;
	    if(i == info->strings->count) break;
	    next = *(string *) arr_get(info->strings, i++);
	}

	if(string_eq(_tag, tag)) {
	    *signature = _signature;
	    *is_signature = !(string_index_of(*signature, "https://") == 0);
	}
    }

    return signature->len;
}

YOUTUBE_DEF bool youtube_info_find_audio(Youtube_Info *info, string *signature, bool *is_signature) {
    *signature = (string) {0};

    size_t i=1;    
    while(i<info->strings->count) {
	i++; //string _tag = *(string *) arr_get(info->strings, i++);
	string type = *(string *) arr_get(info->strings, i++);

	string _signature = {0};
	string next = *(string *) arr_get(info->strings, i++);
	while(!string_eq(next, YOUTUBE_INFO_FRAME)) {
	    string value = *(string *) arr_get(info->strings, i++);
	    if(string_eq_cstr(next, "sig")) _signature = value;
	    if(i == info->strings->count) break;
	    next = *(string *) arr_get(info->strings, i++);
	}

	if(string_index_of(type, "audio/") == 0) {
	    *signature = _signature;
	    *is_signature = !(string_index_of(*signature, "https://") == 0);
	}
    }

    return signature->len;    
}

YOUTUBE_DEF bool youtube_info_find_video(Youtube_Info *info, string *signature, bool *is_signature) {
    *signature = (string) {0};

    size_t i=1;    
    while(i<info->strings->count) {
	i++; //string _tag = *(string *) arr_get(info->strings, i++);
	string type = *(string *) arr_get(info->strings, i++);

	string _signature = {0};
	string next = *(string *) arr_get(info->strings, i++);
	while(!string_eq(next, YOUTUBE_INFO_FRAME)) {
	    string value = *(string *) arr_get(info->strings, i++);
	    if(string_eq_cstr(next, "sig")) _signature = value;
	    if(i == info->strings->count) break;
	    next = *(string *) arr_get(info->strings, i++);
	}

	if(string_index_of(type, "video/") == 0) {
	    *signature = _signature;
	    *is_signature = !(string_index_of(*signature, "https://") == 0);
	}
    }

    return signature->len;    
}

YOUTUBE_DEF string youtube_resposne_match_js_path(string response) {
    string empty = {0};

    const char *prefix = "/s/player/";
    size_t prefix_len = strlen(prefix);
    const char *suffix = "/base.js";
    size_t suffix_len = strlen(suffix);

    int pre_pos = string_index_of(response, prefix);
    if(pre_pos < 0) return empty;

    int suf_pos = string_index_of_offset(response, suffix, (size_t) pre_pos + prefix_len);
    if(suf_pos < 0) return empty;

    return (string) {.data = response.data + pre_pos, .len = suf_pos - pre_pos + suffix_len };
}

//function[:print:]*\\.split\\(\"\"\\)[:print:]*\\.join\\(\"\"\\)}
YOUTUBE_DEF string youtube_response_match_decode_function(string response) {
    string empty = {0};

    const char *prefix = "function";
    size_t prefix_len = strlen(prefix);
    const char *midfix = "split(\"\")";
    size_t midfix_len = strlen(midfix);
    const char *suffix = ".join(\"\")}";
    size_t suffix_len = strlen(suffix);

    size_t offset = 0;

    while(offset < response.len) {
	int pre_pos = string_index_of_offset(response, prefix, offset);
	if(pre_pos < 0) {
	    return empty;
	}	

	offset = (size_t) pre_pos + prefix_len;
	int mid_pos = string_index_of_offset(response, midfix, offset);
	if(mid_pos < 0) {
	    continue;
	}

	bool ok = true;
	for(size_t i=offset;i<(size_t) mid_pos;i++) {
	    if(!regex_printable(response.data[i])) {
		ok = false;
		break;
	    }
	}
	if(!ok) {
	    continue;
	}
	
	offset = (size_t) mid_pos + midfix_len;
	int suf_pos = string_index_of_offset(response, suffix, offset);
	if(suf_pos < 0) {
	    continue;	    
	}

	ok = true;
	for(size_t i=offset;i<(size_t) suf_pos;i++) {
	    if(!regex_printable(response.data[i])) {
		ok = false;
		break;
	    }
	}
	if(!ok) {
	    continue;
	}

	return (string) {.data = response.data + pre_pos, .len = suf_pos - pre_pos + suffix_len };
    }
    
    return empty;
}

//\\.split\\(\"\"\\);[:alnum:]+\\.
YOUTUBE_DEF string youtube_decode_function_match_var_name(string decodeFunction) {
    string empty = {0};

    const char *prefix = ".split(\"\");";
    size_t prefix_len = strlen(prefix);
    const char *suffix = ".";
    size_t suffix_len = strlen(suffix);

    size_t offset = 0;
    while(offset < decodeFunction.len) {
	int pre_pos = string_index_of_offset(decodeFunction, prefix, offset);
	if(pre_pos < 0) {
	    return empty;
	}
	
	offset = (size_t) pre_pos + prefix_len;
	int suf_pos = string_index_of_offset(decodeFunction, suffix, offset);
	if(suf_pos < 0) {	    
	    continue;
	}
	
	bool ok = true;
	for(size_t i=pre_pos+prefix_len;i<(size_t) suf_pos;i++) {
	    if(!regex_alnum(decodeFunction.data[i]) && decodeFunction.data[i]!='$') {
		ok = false;
		break;
	    }
	}
	if(!ok) {
	    continue;
	}

	return (string) {.data = decodeFunction.data + pre_pos, .len = suf_pos - pre_pos + suffix_len };
    }

    return empty;
}

//var "String_Fmt"={.+}};[:alnum:]+\\.[:alnum:]+\\.prototype
YOUTUBE_DEF string youtube_response_match_var_declare_matches(string var_name, string response) {
    string empty = {0};

    const char *prefix = "var ";
    size_t prefix_len = strlen(prefix);
    //var_name
    //var_name_len
    const char *midfix = "={";
    size_t midfix_len = strlen(midfix);
    const char *midfix2 = "}};";
    size_t midfix2_len = strlen(midfix2);
    const char *midfix3 = ".";
    size_t midfix3_len = strlen(midfix3);
    const char *suffix = ".prototype";
    size_t suffix_len = strlen(suffix);

    size_t offset = 0;
    while(offset < response.len) {
	int pre_pos = string_index_of_offset(response, prefix, offset);
	if(pre_pos < 0) {
	    return empty;
	}

	offset = (size_t) pre_pos + prefix_len;
	int var_name_pos = string_index_of_offset2(response, var_name, offset);
	if(var_name_pos < 0) {
	    continue;
	}
	if((size_t) var_name_pos != offset) { // CHECK THIS
	    continue;
	}

	offset = (size_t) var_name_pos + var_name.len;	
	int mid_pos = string_index_of_offset(response, midfix, offset);
	if(mid_pos < 0) {
	    continue;
	}
	if((size_t) mid_pos != offset) {
	    continue;
	}

	offset = (size_t) mid_pos + midfix_len;
	int mid2_pos = string_index_of_offset(response, midfix2, offset);
	if(mid2_pos < 0) {
	    continue;
	}

	offset = (size_t) mid2_pos + midfix2_len;
	int mid3_pos = string_index_of_offset(response, midfix3, offset);
	if(mid3_pos < 0) {
	    continue;
	}
	
	bool ok = true;
	for(size_t i=offset;i<(size_t) mid3_pos;i++) {
	    if(!regex_alnum(response.data[i])) {
		ok = false;
		break;
	    }
	}
	if(!ok) {
	    continue;
	}

	offset = (size_t) mid3_pos + midfix3_len;
	int suf_pos = string_index_of_offset(response, suffix, offset);
	if(suf_pos < 0) {
	    continue;
	}

	ok = true;
	for(size_t i=offset;i<(size_t) suf_pos;i++) {
	    if(!regex_alnum(response.data[i])) {
		ok = false;
		break;
	    }
	}
	if(!ok) {
	    continue;
	}

	return (string) {.data = response.data + pre_pos, .len = suf_pos - pre_pos + suffix_len };	
    }
    
    return empty;
}

YOUTUBE_DEF bool youtube_decoder_init(string response_string, Http *http, String_Buffer *sb, Youtube_Decoder *decoder) {
    size_t sb_len = sb->len;
    
    string js_file_path = youtube_resposne_match_js_path(response_string);
    if(!js_file_path.len) {
	panic("youtube_resposne_match_js_path");
	return false;
    }
    
    const char *js_url = tprintf(sb, String_Fmt, String_Arg(js_file_path));
    sb->len = sb_len;
    if(!http_request(http, js_url,
		     "GET", NULL, NULL, string_buffer_callback, sb, NULL, NULL)) {
	panic("http_get");
    }
    char *jsFile_data = sb->data + sb_len;
    size_t jsFile_len = sb->len - sb_len;

    //decode function
    string decodeFunction_string = youtube_response_match_decode_function( string_from(jsFile_data, jsFile_len) );
    if(!decodeFunction_string.len) {
	panic("youtube_response_match_decode_function");
	return false;
    }

    // -- varNameMatches
    string varNameMatches_string = youtube_decode_function_match_var_name(decodeFunction_string);
    if(!varNameMatches_string.len) {
	printf("decodeFunction: '"String_Fmt"'\n", String_Arg(decodeFunction_string) );
	panic("youtube_decode_function_match_var_name");
	return false;
    }
    string_chop_left(&varNameMatches_string, 11);
    string_chop_right(&varNameMatches_string, 1);

    string var_name_match = youtube_response_match_var_declare_matches(varNameMatches_string, string_from(sb->data + sb_len, jsFile_len) );

    int k = (int) var_name_match.len - 1;
    while(k>=0 && var_name_match.data[k] != '}') k--;
    var_name_match.len = (size_t) k+2;

    decoder->prefix = var_name_match;
    decoder->decodeFunction = decodeFunction_string;

    return true;
}

YOUTUBE_DEF bool youtube_decoder_decode(Youtube_Decoder *decoder, String_Buffer *sb, string signature, const char **stream_url) {
    if(!decoder->duk_ctx) {
	decoder->duk_ctx = duk_create_heap_default();	
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

    if(!url.len) {
	return false;
    }

    const char *expression = tprintf(sb, "\"use-strict\";"String_Fmt"encodeURIComponent(("String_Fmt")(decodeURIComponent(\""String_Fmt"\")));",
				     String_Arg(decoder->prefix), String_Arg(decoder->decodeFunction), String_Arg(__s));
    duk_eval_string(decoder->duk_ctx, expression);

    string decoded_url = tsmap(sb, url, http_decodeURI);

    const char *suffix = duk_get_string(decoder->duk_ctx, -1);
    
    *stream_url = tprintf(sb, String_Fmt"&sig=%s",
			   String_Arg(decoded_url),
			   suffix);  
    return true;

}

YOUTUBE_DEF void youtube_decoder_free(Youtube_Decoder *decoder) {
    duk_destroy_heap(decoder->duk_ctx);
}

YOUTUBE_DEF bool youtube_on_elem_results_init(Json_Parse_Type type, string content, void *arg, void **elem) {
    (void) type;
    (void) content;
    (void) elem;
  
    Youtube_Results *results = (Youtube_Results *) arg;
    results->prev = content;
  
    return true;
}

YOUTUBE_DEF void youtube_on_object_elem_results_init(void *object, string key, void *elem, void *arg) {
    (void) object;
    (void) elem;
    Youtube_Results *results = (Youtube_Results *) arg;
  
    //printf(String_Fmt": "String_Fmt"\n", String_Arg(key), String_Arg(results->prev));

    if(results->state == 0) {
	if(!string_eq_cstr(key, "videoId")) return;
	if(results->videoIds->count == 0) {
	    arr_push(results->videoIds, &results->prev);
	    results->state = 1;
	}
	else {
	    string last_videoId = *(string *) arr_get(results->videoIds, results->videoIds->count-2);
	    if(string_eq(results->prev, last_videoId)) return;
	    arr_push(results->videoIds, &results->prev);
	    results->state = 1;
	}    
    } else if(results->state == 1) {
	if(!string_eq_cstr(key, "title")) return;
	arr_push(results->videoIds, &results->prev);
	results->state = 0;
    }
  
}

YOUTUBE_DEF bool youtube_results_init(string term, Http *http, String_Buffer *sb, Youtube_Results *results) {

    size_t sb_len = sb->len;
    string encoded_term = tsmap(sb, term, http_encodeURI);
    const char *route = tprintf(sb, "/results?search_query="String_Fmt, String_Arg(encoded_term));

    sb->len = sb_len;
    if(!http_request(http, route, "GET", NULL, NULL, string_buffer_callback, sb, NULL, NULL)) {
	return false;
    }
    string response_string = string_from(sb->data + sb_len, sb->len - sb_len);

    Youtube_JavaScript_Content js_content;
    js_content.content = (string) {0};
    js_content.prefix = "var ytInitialData = ";
  
    Html_Parse_Events events = {0};
    events.on_node_content = youtube_on_node_content_query_json;
    events.arg = &js_content;
  
    if(!html_parse(response_string.data, response_string.len, &events)) {
	return false;
    }
    string json_string = js_content.content;
    string_chop_left(&json_string, strlen(js_content.prefix));
    string_chop_right(&json_string, 1);

    *results = (Youtube_Results) {0};
    results->videoIds = arr_init2(sizeof(string), 1024);

    Json_Parse_Events json_events = {0};
    json_events.on_elem = youtube_on_elem_results_init;
    json_events.on_object_elem = youtube_on_object_elem_results_init;
    json_events.arg = results;

    if(!json_parse2(json_string.data, json_string.len, &json_events)) {
	return false;
    }


    return true;
}

YOUTUBE_DEF void youtube_results_dump(Youtube_Results *results) {
    for(size_t i=0;i<results->videoIds->count;i+=2) {
	string videoId = *(string *) arr_get(results->videoIds, i);
	string title = *(string *) arr_get(results->videoIds, i+1);
	printf(String_Fmt" - https://www.youtube.com/watch?v="String_Fmt"\n", String_Arg(title), String_Arg(videoId));
    }
}

YOUTUBE_DEF void youtube_results_free(Youtube_Results *results) {
    arr_free(results->videoIds);
}

#endif //YOUTUBE_IMPLEMENTATION

#endif //YOUTUBE_H_H
