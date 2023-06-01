#define HTTP_IMPLEMENTATION
#include "../libs/http.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../thirdparty/stb_truetype.h"

#define FONT_IMPLEMENTATION
#include "../libs/font.h"

#define GUI_OPENGL
#define IMGUI_RENDERER_IMPLEMENTATION
#include "../libs/imgui.h"

#define THREAD_IMPLEMENTATION
#include "../libs/thread.h"

#define HTML_PARSER_IMPLEMENTATION
#include "../libs/html_parser.h"

#define WIDTH 960
#define HEIGHT 800

String_Buffer html_content = {0};
String_Buffer sb = {0};
size_t lines = 0;

const char *visible_tags[] = {"h1", "p", "a", "div", "span", "iframe", "center", "link", "th", "img", "b"};
const char *other_tags[] = {"style", "title", "script", "path"};

bool on_node(string name, void *arg, void **node) {
    (void) name;
    (void) node;

    string *last = (string *) arg;
    *last = name;
    
    return true;
}

void on_node_content(void *node, string content, void *arg) {
    (void) node;
    (void) content;

    string *last = (string *) arg;    
    if(!last->len) {
	return;	
    }
    for(size_t i=0;i<sizeof(visible_tags)/sizeof(visible_tags[0]);i++) {
	if(string_eq_cstr(*last, visible_tags[i])) {
	    content = string_trim(content);
	    if(!content.len) return;
	    string_buffer_append(&sb, content.data, content.len);
	    string_buffer_append(&sb, "\n", 1);
	    return;
	}
    }

    for(size_t i=0;i<sizeof(other_tags)/sizeof(other_tags[0]);i++) {
	if(string_eq_cstr(*last, other_tags[i])) {
	    return;
	}
    }

    fprintf(stderr, "ERROR: Unknown tag: '"String_Fmt"'\n", String_Arg(*last) ); fflush(stderr);
}

void *request(void *arg) {
    (void) arg;

    if(!http_get("https://de.wikipedia.org/wiki/C_(Programmiersprache)", string_buffer_callback, &html_content, NULL, NULL)) {
	panic("http_get");
    }

    string last = {0};
    
    Html_Parse_Events events = {0};
    events.on_node = on_node;
    events.on_node_content = on_node_content;
    events.arg = &last;
    
    if(!html_parse(html_content.data, html_content.len, &events)) {
	printf("%.*s\n", (int) html_content.len, html_content.data);
	panic("html_parse");
    }

    string content = string_from(sb.data, sb.len);
    while(content.len) {
	string_chop_by_delim(&content, '\n');
	lines++;
    }
	
    return NULL;
}

int main() {
    Gui gui;
    //Gui_Canvas canvas = {WIDTH, HEIGHT, NULL};
    Gui_Canvas canvas = {WIDTH, HEIGHT, NULL};
    Gui_Event event;
    if(!gui_init(&gui, &canvas, "JPlayer")) {
	return 1;
    }
    gui_use_vsync(1);

    if(!imgui_init(&gui, &event)) {
	return 1;
    }

    Font2 font;
    if(!font_init2(&font, "C:\\Windows\\Fonts\\segoeui.ttf", 24)) {
	return 1;
    }
    imgui_set_font(&font);
    imgui_set_background(0xff181818);

    Thread request_thread;
    if(!thread_create(&request_thread, request, NULL)) {
	return 1;
    }

    int scroll = 0;
    float width, height;
    while(gui.running) {
	while(imgui_peek()) {
	    if(event.type == GUI_EVENT_KEYPRESS &&
		event.as.key == 'Q') {
		gui.running = false;
	    } else if(event.type == GUI_EVENT_MOUSEWHEEL) {
		scroll += event.as.amount;
	    }
	}
	imgui_get_size(&width, &height);

	float min = height / (float) font.height - lines;
	if(scroll > 0) scroll = 0;
	//if(scroll < min + 1) scroll = min;
	
	if(!sb.len) {
	    imgui_text( vec2f(0, height - font.height), "Loading ...", vec4f(1, 1, 1, 1) );
	} else {
	    Vec2f pos = vec2f(0, height - font.height*(1 + (float) scroll) );
	    string content = string_from(sb.data, sb.len);
	    while(content.len) {
		string line = string_trim(string_chop_by_delim(&content, '\n'));
		if(pos.y > 0 && pos.y < height) imgui_text_len(pos, line.data, line.len, vec4f(1, 1, 1, 1));
		pos.y -= font.height;
	    }
	}

	imgui_update();
    }

    return 0;
}
