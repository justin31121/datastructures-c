MAKEFLAGS += --no-print-directory

ifeq ($(OS),Windows_NT)
	PREFIX = WIN32
	CLEAN_UP = del *.obj
else
	PREFIX = LINUX
endif

gui: ./src/gui_demo.c ./libs/gui.h
	make $(PREFIX)-gui && $(CLEAN_UP)

WIN32-gui:
	cl ./src/gui_demo.c /Fe:gui.exe opengl32.lib gdi32.lib user32.lib
