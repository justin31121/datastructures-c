#include <stdio.h>
#include <string.h>

#define YOUTUBE_IMPLEMENTATION
#include "../libs/youtube.h"

String_Buffer sb = {0};

int main() {
    string url, name;
    if(!youtube_get_audio2(STRING("D1zowpIVahY"), &sb, &url, &name)) {
	printf("Failed\n");
	return 1;
    }
    
    printf(String_Fmt"\n", String_Arg(url));
    
    return 0;
}
