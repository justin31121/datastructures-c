#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../thirdparty/stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../thirdparty/stb_image_resize.h"

#define IO_IMPLEMENTATION
#include "../libs/io.h"

#include <stdint.h>

typedef struct{
  char *data;
  int size;
  int cap;
}Memory;

void write_func(void *context, void *data, int size) {
  Memory *memory = (Memory *) context;

  if(memory->cap < memory->size + size) {

    int cap = memory->cap == 0 ? size*2 : memory->cap;
    while(cap < memory->size + size) cap *= 2;
    
    memory->data = realloc(memory->data, cap);
    if(!memory->data) {
      fprintf(stderr, "ERORR: Failed to allocate memory\n");
      exit(1);
    }
    memory->cap = cap;
    
  }

  memcpy(memory->data + memory->size, data, size);
  memory->size += size;
}

#pragma pack(push, 1)
typedef struct {
    uint8_t width;       // Image width
    uint8_t height;      // Image height
    uint8_t colorCount;  // Number of colors in the palette (0 for true color ICOs)
    uint8_t reserved;    // Reserved, must be 0
    uint16_t planes;     // Color planes (1)
    uint16_t bpp;        // Bits per pixel (32 for true color with alpha)
    uint32_t imageSize;  // Image size in bytes
    uint32_t imageOffset;// Offset of image data in the file
} IcoImageEntry;
#pragma pack(pop)

int main(int argc, char **argv) {

   if(argc < 3) {
   		fprintf(stderr, "ERROR: Please provide enough arguments\n");
   		fprintf(stderr, "USAGE: %s <image_in> <image_out>\n", argv[0]);
   		return 1;
   }

   const char *input = argv[1];
      const char *output = argv[2];

  int width, height;
  unsigned char *data = stbi_load(input, &width, &height, 0, 4);
  if(!data) {
    fprintf(stderr, "ERORR: Failed to load image\n");
    return 1;
  }

  int out_width = 32;
  int out_height = 32;

  unsigned char *out_data = malloc(out_width * out_height * sizeof(unsigned int));
  if(!out_data) {
    fprintf(stderr, "ERORR: Failed to allocate memory\n");
    return 1;    
  }
  
  if(!stbir_resize_uint8(data, width, height, width * sizeof(unsigned int), out_data, out_width, out_height, out_width * sizeof(unsigned int), 4)) {
    fprintf(stderr, "ERROR: Failed to resize image\n");
    return 1;
  }

  Memory memory = {
    .data = NULL,
    .size = 0,
    .cap = 0,
  };

  if(!stbi_write_png_to_func(write_func, &memory, out_width, out_height, 4, out_data, out_width * sizeof(unsigned int))) {
    fprintf(stderr, "ERROR: Failed to write image\n");
    return 1;
  }

/*
  if(!io_write_file_len("musik_from_memory.png", memory.data, memory.size)) {
    fprintf(stderr, "ERROR: Failed to write_file_len\n");
    return 1;
  }
  */
  
  FILE *file = fopen(output, "wb");
  if(!file) {
    fprintf(stderr, "Failed to open: musik.ico\n");
    return 1;
  }

  uint8_t header[6] = {0, 0, 1, 0, 1, 0};
  IcoImageEntry directoryEntry = {
    .width = out_width,
    .height = out_height,
    .colorCount = 0,
    .reserved = 0,
    .planes = 1,
    .bpp = 32,
    .imageSize = memory.size,
    .imageOffset = sizeof(header) + sizeof(directoryEntry)  // Offset of image data
  };

  fwrite(header, sizeof(header), 1, file);

  // Write the image directory entry
  fwrite(&directoryEntry, sizeof(directoryEntry), 1, file);

  // Write the image data
  fwrite(memory.data, directoryEntry.imageSize, 1, file);

  fclose(file);

  printf("Written ICO to disk!\n");
     
  return 0;
}
