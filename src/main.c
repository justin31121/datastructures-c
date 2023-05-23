#include <stdio.h>
#include <string.h>

#define YOUTUBE_IMPLEMENTATION
#include "../libs/youtube.h"

void foo(string videoId, string tag, Http *http, String_Buffer *sb, Youtube_Info *info ,Youtube_Decoder *decoder) {
    string response;
    if(!youtube_get_response(videoId, http, sb, &response)) {
	panic("youtube_get_response");
    }

    if(!youtube_info_init(response, info)) {
	panic("youtube_info_init");
    }
    //youtube_info_dump(info);

    string signature;
    bool is_signature;
    if(!youtube_info_find_stream(info, tag, &signature, &is_signature)) {
	panic("youtube_info_find_stream");
    }
    printf("sb.len: %zd, sb.cap: %zd\n", sb->len, sb->cap);

    if(!youtube_decoder_init(response, http, sb, decoder)) {
	panic("youtube_decoder_init");
    }
    printf("sb.len: %zd, sb.cap: %zd\n", sb->len, sb->cap);
    
    const char *url;
    if(!youtube_decoder_decode(decoder, sb, signature, &url)) {
	panic("youtube_decoder_decode");
    }

    printf("url: %s\n", url);
}

String_Buffer sb = {0};

int main() {
    string url, name;
    if(!youtube_get_audio2(STRING("g3dmARjiaBc"), &sb, &url, &name)) {
	return 1;
    }
    
    printf(String_Fmt"\n", String_Arg(url));
    
    return 0;
}

int main1() {

    string tag = STRING("140");
    Youtube_Info info;
    Youtube_Decoder decoder;
    decoder.duk_ctx = NULL;
    String_Buffer sb = {0};
    Http http;
    
    char buf[1024];
    while(1) {
	scanf("%s", buf);
	if(strcmp(buf, "q") == 0) break;
	else if(strcmp(buf, "start") == 0) {
	    if(!http_init2(&http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
		panic("http_init2");
	    }
	    string_buffer_reserve(&sb, 4 * 1024 * 1024); 

	    sb.len = 0;	    
	    foo(STRING("g3dmARjiaBc"), tag, &http, &sb, &info, &decoder);
	    sb.len = 0;
	    foo(STRING("D1zowpIVahY"), tag, &http, &sb, &info, &decoder);
	}
	else {
	    string id = string_from_cstr(buf);
	    sb.len = 0;
	    foo(id, tag, &http, &sb, &info, &decoder);
	}
    }
    
    return 0;
}
