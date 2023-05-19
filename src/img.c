#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../thirdparty/stb_image_resize.h"

#define HTTP_IMPLEMENTATION
#include "../libs/http.h"

#include <windows.h>

String_Buffer sb = {0};

int main(int argc, char **argv) {

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);


    int in_width, in_height;
    unsigned int *in_data;
    
    if(argc > 1) {
	if(!http_get(argv[1], string_buffer_callback, &sb, NULL, NULL)) {
	    panic("http_get");
	}

	in_data = (unsigned int *) stbi_load_from_memory((const unsigned char *) sb.data, (int) sb.len, &in_width, &in_height, 0, 4);
	if(!in_data) {
	    fprintf(stderr, "ERROR: Can not load file: %s\n", argv[1]);
	    return 1;
	}	

    } else {
	const char *filename = "./rsc/landscape.jpg";
	in_data = (unsigned int *) stbi_load(filename, &in_width, &in_height, 0, 4);
	if(!in_data) {
	    fprintf(stderr, "ERROR: Can not load file: %s\n", filename);
	    return 1;
	}	
    }

    int width = 60;
    int height = width * in_height / in_width;
    unsigned int *data = malloc(sizeof(unsigned int) * width * height);
    if(!data) {
	fprintf(stderr, "ERROR: Can not allocate enough memory\n");
	return 1;	
    }

    stbir_resize_uint8((const unsigned char *)in_data , in_width , in_height , in_width * sizeof(unsigned int),
		       (unsigned char *) data, width, height, width * sizeof(unsigned int),
		       4);

    for(int j=0;j<height;j++) {
	
	for(int i=0;i<width;i++) {
	    unsigned int pixel = data[j * width + i];
	    int r = (pixel & 0xff);
	    int g = (pixel & 0xff00) >> 8;
	    int b = (pixel & 0xff0000) >> 16;
	    printf("\x1b[48;2;%d;%d;%dm", r, g, b);
	    printf("  ");
	}
	printf("\n");
	//RESET
	printf("\x1b[0m");
    }

    stbi_image_free(data);
    
    return 0;
}
