#include <stdio.h>

#define STRING_IMPLEMENTATION
#include "../libs/string.h"

String_Buffer sb = {0};


int main() {

    long long int space_cap = 1024 * 1024;
    char *space = calloc(1, space_cap);
    if(!space) {
	return 1;
    }

    const char *value = "foooooooooooooooooooooooooooooooooooooooooooooooooooo";
    int pos = cstr_index_of(space, space_cap, value, strlen(value) );
    printf("pos: %d\n", pos);
    
    return 0;
}
