#define IO_IMPLEMENTATION
#include "../libs/io.h"

int main() {

    char *content;
    size_t content_size; 
    if(!io_slurp_file("rsc\\icon.ico", &content, &content_size)) {
	return 1;
    }

    printf("loaded: %d bytes\n", content_size);

    FILE *f = fopen("rsc\\icon.h", "wb");
    
    fprintf(f, "#ifndef ICON_H_H\n");
    fprintf(f, "#define ICON_H_H\n\n");

    fprintf(f, "int icon_size = %d;\n\n", content_size);

    fprintf(f, "unsigned char icon_data[] = {");
    for(size_t i=0;i<content_size;i++) {
	fprintf(f, "%d", (unsigned int) content[i]);
	if(i != content_size - 1) {
	    fprintf(f, ", ");
	}
    }
    fprintf(f, "};\n\n");
    
    fprintf(f, "#endif //ICON_H_H\n");
    
    fclose(f);
    
    return 0;
}
