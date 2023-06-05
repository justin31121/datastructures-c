#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../thirdparty/stb_truetype.h"

#define FONT_IMPLEMENTATION
#include "../libs/font.h"

#include "../libs/util.h"
//#include "../rsc/segoeui.h"

#define OPENGL

#ifdef OPENGL

#define GUI_IMPLEMENTATION
#define GUI_OPENGL
#include "../libs/gui.h"

#define RENDERER_IMPLEMENTATION
#include "../libs/renderer.h"

#define STRING_IMPLEMENTATION
#include "../libs/string.h"

#include "../rsc/img.h"

int main() {
    
    Gui gui;
    if(!gui_init(&gui, NULL, "gui_demo")) {
	return -1;
    }
    gui_use_vsync(1);

    Renderer renderer = {0};
    if(!renderer_init(&renderer)) {
	return -1;
    }

    //Font2 font = font2_segoeui_22;
    //u32 font_tex = renderer_push_texture(&renderer, font.width, font.height, (char *) font.data, true);
    //(void) font_tex;

    u32 background = renderer_push_texture(&renderer, img_width, img_height, (char *) img_data, false);

    Gui_Time timer;
    gui_time_capture(&timer);

    u64 acc_time = 0;  

    Vec2f border;
    Gui_Event event;
    String_Buffer sb = {0};
    while(gui.running) {
      sb.len = 0;
      
	while(gui_peek(&gui, &event)) {
	}

	gui_get_window_sizef(&gui, &border.x, &border.y);

	u64 msPerFrame = gui_time_measure(&timer);
	const char *state = tprintf(&sb, "%lums", msPerFrame);
	printf("%s\n", state);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	//BEGIN RENDERER
	renderer.resolution = border;
	acc_time += msPerFrame;
	renderer.time = (float) acc_time * 0.75f / 1000.f;
	
	renderer.image = background;
	renderer_set_shader(&renderer, SHADER_FOR_IMAGE);
	renderer_image_rect(&renderer,
			    vec2f(0, 0),
			    border,
			    vec2f(0, 0),
			    vec2f(1, 1));
	renderer_flush(&renderer);

	/*
	renderer.image = font_tex;
	renderer_set_shader(&renderer, SHADER_FOR_TEXT);
	const char *word = stateBuffer;

	stbtt_aligned_quad q;
	float x0= 0;
	float y0= border.y - (float) font.height;
    
	s32 k = 0;
	while(word[k]) {
	    char c = word[k++];

	    if(c >= 32 && c < 128) {
	      stbtt_GetBakedQuad((stbtt_bakedchar *) font.cdata, 512, 512, c-32, &x0, &y0, &q, 1);

	      renderer_text(&renderer,
			    vec2f(q.x0, 2*y0 - q.y1),
			    vec2f(q.x1 - q.x0, q.y1 - q.y0),
			    vec2f(q.s0, 1 - q.t1),
			    vec2f(q.s1 - q.s0, q.t1 - q.t0),
			    
			    vec4f(1, 1, 1, 1));
	    }
	}
	renderer_flush(&renderer);
	*/
	
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

int main() {
    Font2 font;
    if(!font_init2(&font, "C:\\Windows\\Fonts\\arial.ttf", 24)) {
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
	font_render2(&font, word, strlen(word),
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
    if(!gui_init(&gui, &canvas, "gui_demo")) {
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
    font_free2(&font);

    return 0;
}

#endif //OPENGL
