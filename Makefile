MAKEFLAGS += --no-print-directory

GCC-FLAGS = -Wall -Wextra -pedantic -Wshadow -ggdb

ifeq ($(OS),Windows_NT)
	PREFIX = CL
	GUI = -lgdi32
	OPENGL = -lopengl32
	AUDIO = -lxaudio2_8 -lole32
else
	PREFIX = GCC
	GUI = -lX11
	OPENGL = -lGLX -lGL

	LIBS = alsa
	LDFLAGS = `pkg-config --libs $(LIBS)`
	LDLIBS = `pkg-config --cflags $(LIBS)`
endif

ifeq ($(PREFIX),CL)
	CLEAN_UP = && del *.obj
endif

#==========================================================================

imgui_demo: ./src/imgui_demo.c
	make $(PREFIX)-imgui_demo $(CLEAN_UP)

CL-imgui_demo:
	cl ./src/imgui_demo.c /Fe:imgui.exe

#==========================================================================

player_demo: ./src/player_demo.c ./libs/player.h
	make $(PREFIX)-player_demo $(CLEAN_UP)

CL-player_demo:
	cl ./src/player_demo.c /Fe:player.exe

GCC-player_demo:
	gcc ./src/player_demo.c ./thirdparty/duktape.o $(GCC-FLAGS) -o player $(GUI) $(OPENGL) -lavutil -lavcodec -lavformat -lswresample $(AUDIO) -lm -lpthread $(LDFLAGS) $(LDLIBS) -lssl -lcrypto -ggdb

#==========================================================================

get_demo: ./src/get_demo.c ./libs/http.h
	make $(PREFIX)-get_demo $(CLEAN_UP)

GCC-get_demo:
	gcc ./src/get_demo.c $(GCC-FLAGS) -o get_demo -lpthread -lssl -lcrypto -lws2_32

#==========================================================================

google: ./src/google.c ./libs/http.h ./libs/http.h ./libs/util.h ./libs/string.h ./libs/tokenizer.h ./libs/html_parser.h
	make $(PREFIX)-google $(CLEAN_UP)

GCC-google:
	gcc ./src/google.c $(GCC-FLAGS) -o google -lssl -lcrypto -lws2_32

CL-google:
	cl ./src/google.c /Fe:google.exe

#==========================================================================

httpserver: ./libs/http.h ./src/httpserver.c
	make $(PREFIX)-httpserver $(CLEAN_UP)

CL-httpserver:
	cl ./src/httpserver.c /Fe:httpserver.exe

#==========================================================================

gui: ./src/gui_demo.c ./libs/gui.h ./libs/font.h
	make $(PREFIX)-gui $(CLEAN_UP)

GCC-gui:
	gcc ./src/gui_demo.c $(GCC-FLAGS) -o gui $(GUI) $(OPENGL) -lm

CL-gui:
	cl ./src/gui_demo.c /Fe:gui.exe
