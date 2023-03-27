#ifndef FONT_H_
#define FONT_H_

#include <stdio.h>

#ifndef FONT_DEF
#define FONT_DEF static inline
#endif //FONT_DEF

typedef struct{
  u8 *data;          //data of glyphs
  int *xs, *ys, *ws; //glyph x-/y-offset + glyph width
  unsigned int width, height;
}Font;

FONT_DEF bool font_init(Font *font, const char* font_path, int font_height);
FONT_DEF unsigned int font_estimate_width(const Font *font, const char *cstr);
FONT_DEF void font_free(Font *font);

#ifdef FONT_IMPLEMENTATION

#ifdef STB_TRUETYPE_IMPLEMENTATION

FONT_DEF char *font_slurp_file(const char* file_path, size_t *size) {
  char *res = NULL;
  
  FILE *f = fopen(file_path, "rb");
  if(!f) {
    fprintf(stderr, "[WARNING]: Can not open file '%s' because: %s\n", file_path, strerror(errno));
    return NULL;
  }

  if(fseek(f, 0, SEEK_END) < 0) {
    fprintf(stderr, "[WARNING]: Can not read file '%s' because: %s\n", file_path, strerror(errno));
    fclose(f);
    return NULL;
  }

  long m = ftell(f);
  if(m < 0) {
    fprintf(stderr, "[WARNING]: Can not read file '%s' because: %s\n", file_path, strerror(errno));
    fclose(f);
    return NULL;
  }  

  if(fseek(f, 0, SEEK_SET) < 0) {
    fprintf(stderr, "[WARNING]: Can not read file '%s' because: %s\n", file_path, strerror(errno));
    fclose(f);
    return NULL;
  }

  res = (char *) malloc((size_t) m + 1);
  if(!res) {
    fprintf(stderr, "[WARNING]: Can not allocate enough memory for file '%s'\n", file_path);
    fclose(f);
    return NULL;
  }

  size_t _m = (size_t) m;
  size_t n = fread(res, 1, _m, f);
  if(n != _m) {
    fprintf(stderr, "[WARNING]: Can not read file '%s' because: %s\n", file_path, strerror(errno));
    fclose(f);
    exit(1);    
  }
  res[n] = 0;

  if(size) *size = n;

  fclose(f);
  return res;
}


FONT_DEF bool font_init(Font *font, const char* font_path, int font_height) {
    if(!font) {
    return false;
  }
  
  char *content = font_slurp_file(font_path, NULL);
  if(!content) {
    return false;
  }

  unsigned int ascii_vals = 127 - 32;
  unsigned int ascii_width = ascii_vals * font_height;
  //data: ascii_vals * font_height * font_height * sizeof(u8)
  //xs, ys, ws: 3 * acsii_vals * sizeof(int)

  u8 *data = malloc(ascii_vals * font_height * font_height * sizeof(u8) + // font->data
		    3 * ascii_vals * sizeof(unsigned int)); // font->xs, font->ys, font->ws
  if(!data) {
    free(content);
    return false;
  }

  //partition allocated data
  font->data = data;
  data += ascii_vals * font_height * font_height * sizeof(u8);
  font->xs = (int *) data;
  data += ascii_vals * sizeof(unsigned int);
  font->ys = (int *) data;
  data += ascii_vals * sizeof(unsigned int);
  font->ws = (int *) data;

  //load glyphs
  stbtt_fontinfo font_info = {0};
  stbtt_InitFont(&font_info, (u8 *) content, 0);
  
  float scale = stbtt_ScaleForPixelHeight(&font_info, font_height);
  int _font_height = (int) font_height;

  for(int c=32;c<=126;c++) {
    unsigned int glyph_off = (c-32)*font_height;
    
    int w, h, x ,y;
    u8 *bitmap =
      stbtt_GetCodepointSDF(&font_info, scale, c, 0, 64, 128.0, &w, &h, &x, &y);
      //stbtt_GetCodepointBitmap(&font_info, 0, scale, c, &w, &h, &x, &y);

    font->xs[c - 32] = x;
    font->ys[c - 32] = y;
    font->ws[c - 32] = w;
    //font->hs[c-32] = h;    
    
    for (int j=0; j< _font_height; ++j) {
      for (int i=0; i < _font_height; ++i) {
	if(bitmap && i<w && j<h) {
	  font->data[j*ascii_width+glyph_off+i] = bitmap[j*w+i];
	} else {
	  font->data[j*ascii_width+glyph_off+i] = 0x00;
	}
      }
    }
    
    if(bitmap) stbtt_FreeBitmap(bitmap, font_info.userdata);
  }

  font->height = font_height;
  font->width = ascii_width;
  
  free(content);
  return true;
}

#endif //STB_TRUETYPE_IMPLEMENTATION

FONT_DEF unsigned int font_estimate_width(const Font *font, const char *cstr) {
  unsigned int x0 = 0;
  while(*cstr) {
    char c = *cstr++;
    if(c == ' ') x0 += font->height * 1 / 4;
    else x0 += font->xs[c - 32] + font->ws[c - 32];
  }
  return x0;
}

FONT_DEF void font_free(Font *font) {
  if(!font) {
    return;
  }

  free(font->data);
}

#endif //FONT_IMPLEMENTATION

#endif //FONT_H_