#include <stdio.h>

#define BUILD_IMPLEMENTATION
#include "../libs/build.h"

int main(int argc, char ** argv) {
    rebuild_urself(argc, argv);
    
    return run("gcc", "-o", "gui", "./src/gui_demo.c", "-lgdi32", "-lopengl3");
}
