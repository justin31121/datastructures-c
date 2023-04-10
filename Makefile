MAKEFLAGS += --no-print-directory

GCC-FLAGS = -Wall -Wextra -pedantic -Wshadow -ggdb

PREFIX = CL

ifeq ($(PREFIX),CL)
	CLEAN_UP = && del *.obj
endif

#==========================================================================

player_demo: ./src/player_demo.c ./libs/player.h
	make $(PREFIX)-player_demo $(CLEAN_UP)

CL-player_demo:
	cl ./src/player_demo.c /Fe:player.exe

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

#==========================================================================

httpserver: ./libs/http.h ./src/httpserver.c
	make $(PREFIX)-httpserver $(CLEAN_UP)

CL-httpserver:
	cl ./src/httpserver.c /Fe:httpserver.exe

#==========================================================================

gui: ./src/gui_demo.c ./libs/gui.h ./libs/font.h
	make $(PREFIX)-gui $(CLEAN_UP)

GCC-gui:
	gcc ./src/gui_demo.c $(GCC-FLAGS) -o gui -lopengl32 -lgdi32

CL-gui:
	cl ./src/gui_demo.c /Fe:gui.exe
