#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../thirdparty/stb_truetype.h"

#define FONT_IMPLEMENTATION
#include "../libs/font.h"

#include "../libs/util.h"

#define OPENGL

#ifdef OPENGL

#define GUI_OPENGL
#define GUI_IMPLEMENTATION
#include "../libs/gui.h"

#define RENDERER_IMPLEMENTATION
#include "../libs/renderer.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void) hPrevInstance;
    (void) lpCmdLine;
    
    Gui gui;
    if(!gui_init(&gui, NULL, hInstance, nCmdShow, "gui_demo")) {
	return -1;
    }
    gui_use_vsync(1);

    Renderer renderer = {0};
    if(!renderer_init(&renderer)) {
	return -1;
    }

    Font font;
    if(!font_init(&font, "C:\\Windows\\Fonts\\arial.ttf", 32)) {
	return -1;
    }

    Renderer_Texture font_texture = {font.width, font.height, (char *) font.data, true};
    u32 font_tex = renderer_push_texture(&renderer, font_texture);

    Renderer_Texture background_texture = {0};
    background_texture.data = (s8 *) stbi_load("./img.jpg", &background_texture.width,
					       &background_texture.height, 0, 4);
    u32 background = renderer_push_texture(&renderer, background_texture);
    stbi_image_free(background_texture.data);

    Gui_Time timer;
    gui_time_capture(&timer);

    u64 acc_time = 0;  

    Vec2f border;
    Gui_Event event;
    while(gui.running) {
    
	gui_get_window_sizef(&gui, &border.x, &border.y);

	u64 msPerFrame = gui_time_measure(&timer);
	char stateBuffer[1024];
	if(snprintf(stateBuffer, 1024, "%lums", msPerFrame) >= 1024) {
	    panic("buffer overflow");
	}
	
	while(gui_peek(&gui, &event)) {
	    if(event.msg.message == WM_KEYDOWN) {
		gui_toggle_fullscreen(&gui);
	    }
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	//BEGIN RENDERER
	renderer.resolution = border;
	acc_time += msPerFrame;
	renderer.time = (float) acc_time * 0.75f / 1000.f;

	renderer.image = background;
	renderer_set_shader(&renderer, SHADER_FOR_RIPPLE);
	renderer_image_rect(&renderer, vec2f(0, 0), border, vec2f(0, 0), vec2f(1, 1));
	renderer_flush(&renderer);

	renderer.image = font_tex;
	renderer_set_shader(&renderer, SHADER_FOR_TEXT);

	const char *word = stateBuffer;
	float word_x = 0;
	float word_y = border.y - font.height*2;
    
	s32 k = 0;
	while(word[k]) {
	    char c = word[k++];
	    if(c == ' ') {
		word_x += font.height / 3;
		continue;
	    }
	
	    float char_width = (float) font.ws[c - 32] / (float) font.width;
	    float char_off = (float) (c - 32) * (float) font.height / (float) font.width;

	    Vec2f char_size = vec2f(font.ws[c - 32], font.height);
	    word_x += font.xs[c -32];

	    renderer_text(&renderer,
			  vec2f(word_x, word_y - font.ys[c - 32]),
			  char_size,
			  vec2f(char_off, 0),
			  vec2f(char_width, 1),
			  vec4f(1, 1, 1, 1));
	    word_x += char_size.x;
	}
	renderer_flush(&renderer);    

	renderer_set_shader(&renderer, SHADER_FOR_COLOR);
	renderer_solid_rect(&renderer, vec2f(0, 0), vec2f(100, 100), vec4f(0, 1, 0, 1));
	renderer_flush(&renderer);
	//END RENDERER

	gui_swap_buffers(&gui);
    }
  
    gui_free(&gui);
  
    return 0;
}

#else

#define GUI_IMPLEMENTATION
#include "../libs/gui.h"

////////////////////////////////////

#define PEN_IMPLEMENTATION
#include "../libs/pen.h"

#define WIDTH 1280
#define HEIGHT 720

static unsigned int pixels[WIDTH * HEIGHT];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void) hPrevInstance;
    (void) lpCmdLine;

    Font font;
    if(!font_init(&font, "C:\\Windows\\Fonts\\arial.ttf", 24)) {
	return -1;
    }

    s32 img_w, img_h;
    u32 *img = (u32 *) stbi_load("./img.jpg", &img_w, &img_h, NULL, 4);
    abgr_to_argb(img, img_w, img_h);
  
    fill(pixels, WIDTH, HEIGHT, 0xff181818);
    copy2(pixels, WIDTH, HEIGHT, img, img_w, img_h, WIDTH - img_w / 4, 0, img_w / 4, img_h / 4);
    char *word = "The quick brown fox jumps over the lazy dog!?.";
    u8 n = 20;
    for(u8 i=1;i<n+1;i++) {
	font_render(&font, word, strlen(word),
		    pixels, WIDTH, HEIGHT,
		    0, HEIGHT - i*font.height, ((0xff * (i-1) / n) << 24) | 0x00ffffff);
    }
    rect(pixels, WIDTH, HEIGHT, 100, 100, 400, 550, 0x44ffdd33);
    circle(pixels, WIDTH, HEIGHT, WIDTH/2, HEIGHT/2, 100, 0x9933ddff);
    triangle(pixels, WIDTH, HEIGHT,
	     WIDTH - 30, HEIGHT - 30,
	     WIDTH - 30, HEIGHT - 200 - 30,
	     WIDTH - 400 - 30, HEIGHT - 30, 0xffff0000);

    Gui gui;
    Gui_Canvas canvas = {WIDTH, HEIGHT, pixels};
    if(!gui_init(&gui, &canvas, hInstance, nCmdShow, "gui_demo")) {
	return -1;
    }

    Gui_Event event;
    while(gui.running) {
    
	while(gui_peek(&gui, &event)) {
	}
    
	gui_render_canvas(&gui);
    }

    gui_free(&gui);
    stbi_image_free(img);
    font_free(&font);

    return 0;
}

#endif //OPENGL
