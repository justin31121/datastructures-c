#include <stdio.h>

#define YOUTUBE_IMPLEMENTATION
#include "../libs/youtube.h"

int main(int argc, char **argv) {

    if(argc < 2) {
	fprintf(stderr, "ERROR: Please provide a searchterm\n");
	fprintf(stderr, "USAGE: %s <searchterm>\n", argv[0]);
	exit(1);
    }

    Http http;
    const char *hostname = "www.youtube.com";
    if(!http_init2(&http, hostname, strlen(hostname), true)) {
	panic("http_init2");
    }

    Json out;
    if(!youtube_search(&http, argv[1], &out)) {
	panic("youtube_search");
    }

    for(size_t i=0;i<json_size(out);i++) {
	Json result = json_opt(out, i);
	if(result.type != JSON_OBJECT) {
	    continue;
	}

	const char *videoId = json_get_string(result, "videoId");
	const char *title = json_get_string(
	    json_opt(json_get(json_get(result, "title"), "runs"), 0), "text");
       
	printf("'%s' https://www.youtube.com/watch?v=%s\n", title, videoId);
    }

    return 0;
}
