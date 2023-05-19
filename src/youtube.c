#define LIBAV_IMPLEMENTATION
#include "../libs/libav.h"

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

string FRAME = STRING_STATIC("FRAME");

typedef struct{
    string prev;
    Arr *videoIds;
    int state;
}Player_Results;

bool on_elem_search(Json_Parse_Type type, string content, void *arg, void **elem) {
    (void) type;
    (void) content;
    (void) elem;
  
    Player_Results *results = (Player_Results *) arg;
    results->prev = content;
  
    return true;
}

void on_object_elem_search(void *object, string key, void *elem, void *arg) {
    (void) object;
    (void) elem;
    Player_Results *results = (Player_Results *) arg;
  
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
		arr_push(foo->strings, &FRAME);
		arr_push(foo->strings, &foo->prev_prev);
		arr_push(foo->strings, &content);
		arr_push(foo->strings, &foo->prev);
	    } else {
		foo->state = 1;
		arr_push(foo->strings, &FRAME);
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

    if(foo->state == 1) {
	arr_push(foo->strings, &key);
	arr_push(foo->strings, &foo->prev);
    }
}

typedef struct{
    string content;
    const char *prefix;
}JavaScript_Content;

void on_node_content(void *_node, string content, void *arg) {
    (void) _node;
    JavaScript_Content *js_content = (JavaScript_Content *) arg;
    if(string_index_of(content, js_content->prefix) != 0) return;
    js_content->content = content;
}

//Will discard everything allocated in sb
//Returns json stored in sb
bool query_json(Http *http, const char *route, const char *name, String_Buffer *sb, string *initialPlayerResponseString) {

    size_t sb_len = sb->len;
    if(!http_request(http, route, "GET", NULL, NULL, string_buffer_callback, sb, NULL, NULL)) {
	return false;
    }

    JavaScript_Content js_content;
    js_content.content = (string) {0};
    js_content.prefix = name;
  
    Html_Parse_Events events = {0};
    events.on_node_content = on_node_content;
    events.arg = &js_content;
  
    if(!html_parse(sb->data + sb_len, sb->len - sb_len, &events)) {
	return false;
    }
    string json_string = js_content.content;
    
    string_chop_left(&json_string, strlen(name)); // 'var ytInitialData = '
    string_chop_right(&json_string, 1); // ';'

    *initialPlayerResponseString = json_string;
    return true;
}

bool search(const char *term, String_Buffer *sb) {

    sb->len = 0;
  
    char buf[1024];
    size_t buf_size;
    if(!http_encodeURI(term, strlen(term), buf, 1023, &buf_size)) {
	return false;
    }
    buf[buf_size] = 0;

    Http http;
    if(!http_init2(&http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
	return false;
    }

    const char *route = tprintf(sb, "/results?search_query=%s", buf);
    string initialData;
    if(!query_json(&http, route, "var ytInitialData = ", sb, &initialData)) {
	return false;
    }

    Player_Results results = {0};
    results.videoIds = arr_init2(sizeof(string), 1024);
  
    Json_Parse_Events events = {0};
    events.on_elem = on_elem_search;
    events.on_object_elem = on_object_elem_search;
    events.arg = &results;

    if(!json_parse2(initialData.data, initialData.len, &events)) {
	return false;
    }

    for(size_t i=0;i<results.videoIds->count;i+=2) {
	string videoId = *(string *) arr_get(results.videoIds, i);
	string title = *(string *) arr_get(results.videoIds, i+1);
	printf(String_Fmt" - https://www.youtube.com/watch?v="String_Fmt"\n", String_Arg(title), String_Arg(videoId));
    }

    return true;
}

bool get_streams(Player_Info *info, Http *http, String_Buffer *sb, const char *youtube_url) {

    const char *route = youtube_url + 8 + strlen(YOUTUBE_HOSTNAME);
  
    string response;
    if(!query_json(http, route, PREFIX, sb, &response)) {
	return false;
    }
 
    info->state = 0;
    info->strings = arr_init2(sizeof(string), 1024);
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
    (void) info;
    (void) itag;
    (void) is_signature;

    size_t i=1;    
    while(i<info->strings->count) {
	string tag = *(string *) arr_get(info->strings, i++);
	i++; //string type = *(string *) arr_get(info->strings, i++);

	string _signature = {0};
	string next = *(string *) arr_get(info->strings, i++);
	while(!string_eq(next, FRAME)) {
	    string value = *(string *) arr_get(info->strings, i++);
	    if(string_eq_cstr(next, "sig")) _signature = value;
	    if(i == info->strings->count) break;
	    next = *(string *) arr_get(info->strings, i++);
	}

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

    int k = (int) len-1;
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
    av_log_set_level(AV_LOG_QUIET);

    State state = STATE_NONE;
    bool download = false;
    bool pack = false;
    const char *out_name = NULL;

    string itag = {0};
    const char *program = next(&argc, &argv);

    const char *in = next(&argc, &argv);  
    if(in == NULL) {
	fprintf(stderr, "ERROR: Please provide a searchterm or an url\n");
	fprintf(stderr, "USAGE: %s [-o <output_name>] (--pack / [-d] [ (-f <tag>) / (-audio/-video)]) <searchterm/url>\n", program);
	exit(1);
    }

    if(strcmp(in, "-o") == 0) {
	out_name = next(&argc, &argv);
	in = next(&argc, &argv);
    }

    if(strcmp(in, "--pack") == 0) {
	pack = true;
	state = STATE_AUDIO; //search for audio reguarly and video specially
	in = next(&argc, &argv);
    }

    if(!pack && strcmp(in, "-d") == 0) {
	download = true;
	in = next(&argc, &argv);
    }

    if(!pack && strcmp(in, "-f") == 0) {
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
    if(!youtube_get_videoId(term, &videoId)) {
	search(term, &sb);
	return 0;
    }  
    
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
  
    string itag_video = {0}; // this is only used for pack==true
    if(!itag.len) {

	const char *display[] = {"width", "height", "fps", "audioSampleRate"};

	size_t i=1;
	int best_width = -1;
	while(i<info.strings->count) {
	    string tag = *(string *) arr_get(info.strings, i++);
	    string type = *(string *) arr_get(info.strings, i++);

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

	    string width = {0};
	    string next = *(string *) arr_get(info.strings, i++);
	    while(!string_eq(next, FRAME)) {
		string value = *(string *) arr_get(info.strings, i++);
		if(string_eq_cstr(next, "width")) width = value;

		bool hide = true;
		size_t display_len = sizeof(display)/sizeof(display[0]);
		for(size_t j=0;j<display_len;j++) {
		    if(string_eq_cstr(next, display[j])) {
			hide = false; break;
		    }
		}
		if(!hide && state == STATE_NONE) {
		    printf("\t"String_Fmt":'"String_Fmt"'\n", String_Arg(next), String_Arg(value));
		}
	
		if(i == info.strings->count) break;
		next = *(string *) arr_get(info.strings, i++);
	    }

	    if(pack && string_index_of(type, "video/") == 0) {
		int n;
		string_chop_int(&width, &n);
		if(n <= 1920 && n > best_width) {
		    itag_video = tag;
		    best_width = n;
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
    bool is_signature = false;
    if(!find_stream(&info, itag, &stream, &is_signature)) {
	panic("find_stream");
    }

    const char *url_video = NULL;
    string stream_video = {0};
    bool is_signature_video = false;
    if(pack) {
	if(!find_stream(&info, itag_video, &stream_video, &is_signature_video)) {
	    panic("find_stream");
	}    
    }
  
    Decoder decoder;
    char decoded_url[1024 * 2];
    size_t decoded_url_size;
    char decoded_url_video[1024 * 2];
    size_t decoded_url_size_video;
    if(is_signature || is_signature_video) {
	if(!build_decoder(&decoder, &http, string_from(sb.data, sb.len))) {
	    panic("build decoder");
	}
    }

    if(is_signature) {
	if(!decode(&decoder, stream, &url)) {
	    panic("decode");
	}
    } else {
	if(!http_decodeURI(stream.data, stream.len, decoded_url, 1023 * 2, &decoded_url_size)) {
	    panic("decoded_url overflow");
	}
	decoded_url[decoded_url_size] = 0;
	url = decoded_url;    
    }

    if(pack) {
	if(is_signature_video) {
	    if(!decode(&decoder, stream_video, &url_video)) {
		panic("decode");
	    }
	} else {
	    if(!http_decodeURI(stream_video.data, stream_video.len, decoded_url_video, 1023 * 2, &decoded_url_size_video)) {
		panic("decoded_url overflow");
	    }
	    decoded_url_video[decoded_url_size_video] = 0;
	    url_video = decoded_url_video;
	}
    }

    if(pack) {
	//TODO: Multithreaded audio and video download
	//And handle the logging somehow

	out_name = "temp_audio.m4a";
	char *out_video_name = "temp_video.mp4";

	{      
	    char *out_buffer;
	    size_t out_buffer_size;
	    printf("Url-Audio: '%s'\n", url);
      
	    download3(url, true, &out_buffer, &out_buffer_size);

	    io_write_file_len(out_name, out_buffer, out_buffer_size);
	    printf("Saved: '%s'\n", out_name);
	}

	thread_sleep(500);

	{      
	    char *out_buffer;
	    size_t out_buffer_size;
	    printf("Url-Video: '%s'\n", url_video);
      
	    download3(url_video, true, &out_buffer, &out_buffer_size);

	    io_write_file_len(out_video_name, out_buffer, out_buffer_size);
	    printf("Saved: '%s'\n", out_video_name);
	}

	if(!libav_merge(out_video_name, out_name, "temp.mp4")) {
	    panic("libav_merge");
	}
	io_delete(out_name);
	io_delete(out_video_name);

    } else if(download) {
	char *out_buffer;
	size_t out_buffer_size;
	printf("Url: '%s'\n", url);
      
	download3(url, true, &out_buffer, &out_buffer_size);

	if(!out_name) {
	    out_name = "videoplayback.mp4";
	}
	io_write_file_len(out_name, out_buffer, out_buffer_size);
	printf("Saved: '%s'\n", out_name);
    } else {
	printf("%s\n", url);
    }
  
    return 0;
}
