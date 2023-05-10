#include <stdio.h>

#define BUILD_IMPLEMENTATION
#include "./libs/build.h"

#define STRING_IMPLEMENTATION
#include "./libs/string.h"

int main(int argc, char ** argv) {

    if(argc < 2) {
	fprintf(stderr, "ERROR: no default target\n");
	return 1;
    }
    const char *arg = argv[1];

    bool use_gcc = linux() || !msvc();

    //DEFAULT LINUX
    const char *link_libav = "-lavformat -lavcodec -lavutil -lswresample";
    const char *link_ssl = "-lssl -lcrypto";
    
    const char *link_video = "-lX11 -lGLX -lGL"; //X11, opengl
    const char *link_audio = "";
    const char *link_sockets = "";
    if(windows()) {
	link_ssl = "-lssl -lcrypto -lws2_32";
	
	link_sockets = "-lws2_32";
	link_video = "-lgdi32 -lopengl32"; //gdi, opengl
	link_audio = "-lole32 -lxaudio2_8"; //xaudio
    }

    //DEFAULT GCC
    const char *obj_duktape = "./thirdparty/duktape.o";
    if(msvc()) {
	obj_duktape = "./thirdparty/duktape.obj";
    }
    int ret = 0;

    //DEFAULT GCC
    const char *flags = "-Wall -Wextra -Wshadow -pedantic -ggdb";
    if(msvc()) {
	flags = "";
    }

    if(strcmp(arg, "player_demo") == 0) {
	if(use_gcc) {
	    if(!io_exists(obj_duktape, NULL)) {
		ret = run("gcc -o ", obj_duktape,"-c ./thirdparty/duktape.c");
		if(ret) return ret;
	    }	    
	    ret = run("gcc ", flags, "-o player", obj_duktape, "./src/player_demo.c",
		link_ssl, link_libav, link_video, link_audio);
	} else {
	  if(!io_exists(obj_duktape, NULL)) {
	    ret = run("cl /Fe:", obj_duktape,"/c ./thirdparty/duktape.c");
	    if(ret) return ret;
	  }	    
	  ret = run("cl", flags, "/Fe:player", obj_duktape, "./src/player_demo.c");
	}
    } else if(strcmp(arg, "gui_demo") == 0) {
	if(use_gcc) {
	    ret = run("gcc", flags, "-o gui ./src/gui_demo.c", link_video);
	}
    } else if(strcmp(arg, "youtube") == 0) {
	if(use_gcc) {
	    if(!io_exists(obj_duktape, NULL)) {
		ret = run("gcc -o ", obj_duktape,"-c ./thirdparty/duktape.c");
		if(ret) return ret;
	    }	    
	    ret = run("gcc", flags, "-o youtube ", obj_duktape, "./src/youtube.c", link_ssl);
	} else {
	  if(!io_exists(obj_duktape, NULL)) {
	    ret = run("cl /Fe:", obj_duktape,"/c ./thirdparty/duktape.c");
	    if(ret) return ret;
	  }	    
	  ret = run("cl", flags, "/Fe:youtube", obj_duktape, "./src/youtube.c");
	}
    } else if(strcmp(arg, "google") == 0) {
	if(use_gcc) {
	    ret = run("gcc", flags, "-o google ./src/google.c", link_ssl);
	}
    } else if(strcmp(arg, "get_demo") == 0) {
	if(use_gcc) {
	    ret = run("gcc", flags, "-o get_demo ./src/get_demo.c", link_ssl);
	}
    } else if(strcmp(arg, "httpserver") == 0) {
	if(use_gcc) {
	    ret = run("gcc", flags, "-o httpserver ./src/httpserver.c", link_sockets);
	}
    } else if(strcmp(arg, "jjson") == 0) {
	if(use_gcc) {
	    ret = run("gcc", flags, "-o jjson ./src/jjson.c", link_ssl);
	}	
    } else if(strcmp(arg, "regex_demo") == 0) {
	if(use_gcc) {
	    ret = run("gcc", flags, "-o regex ./src/regex_demo.c");
	} else {
	  ret = run("cl", flags, "/Fe:regex ./src/regex_demo.c");
	}
    } else if(strcmp(arg, "imgui_demo") == 0) {
	if(use_gcc) {
	    ret = run("gcc", flags, "-o imgui ./src/imgui_demo.c", link_video);
	} else {
	  ret = run("cl", flags, "/Fe:imgui ./src/imgui_demo.c");
	}
    } else {	
	fprintf(stderr, "ERROR: Unknown target: '%s'\n", arg);
	return 1;
    }

    if(msvc()) {
      Io_Dir dir;
      if(io_dir_open(&dir, ".\\")) {
	
	Io_File file;
	const char * target = ".obj";
	int target_len = strlen(target);
	while(io_dir_next(&dir, &file)) {
	  int name_len = strlen(file.abs_name);
	  int pos = cstr_index_of(file.abs_name, name_len, target, target_len);
	  if(pos == -1 || name_len != pos + target_len) continue;

	  io_delete(file.abs_name);
	}
	
	io_dir_close(&dir);
      }

    }
    
    return ret;
}
