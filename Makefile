MAKEFLAGS += --no-print-directory

ifeq ($(OS),Windows_NT)
	PREFIX = CL
	CLEAN_UP = && del *.obj
	CL_MISSING = where cl 2> NUL
else
	CFLAGS = -Wall -Wextra -pedantic -Wshadow -ggdb
	PREFIX = GCC
endif

ifdef CL_MISSING
	PREFIX = GCC
	undefine CLEAN_UP
endif

#==========================================================================

google: ./src/google.c ./libs/http.h ./libs/http.h ./libs/util.h ./libs/string.h ./libs/tokenizer.h ./libs/html_parser.h
	make $(PREFIX)-google $(CLEAN_UP)

GCC-google:
	gcc ./src/google.c $(CFLAGS) -o google -lssl -lcrypto -lws2_32

#==========================================================================

gui: ./src/gui_demo.c ./libs/gui.h
	make $(PREFIX)-gui $(CLEAN_UP)

GCC-gui:
	gcc ./src/gui_demo.c $(CFLAGS) -o gui -lopengl32 -lgdi32

CL-gui:
	cl ./src/gui_demo.c /Fe:gui.exe opengl32.lib gdi32.lib user32.lib
