#ifndef FONT_H_
#define FONT_H_

#include <stdio.h>

#ifdef FONT_IMPLEMENTATION
#define PEN_IMPLEMENTATION
#define IO_IMPLEMENTATION
#endif //FONT_IMPLEMENTATION

#include "./pen.h"
#include "./io.h"

#ifndef FONT_DEF
#define FONT_DEF static inline
#endif //FONT_DEF

typedef struct {
   unsigned short x0,y0,x1,y1; // coordinates of bbox in bitmap
   float xoff,yoff,xadvance;
} font_stbtt_bakedchar;

#if 0

typedef struct{
  unsigned char *bitmap;
  font_stbtt_bakedchar *cdata;
  unsigned int height;
}Font;

FONT_DEF bool font_init(Font *font, const char* font_path, int font_height);
FONT_DEF unsigned int font_estimate_width(const Font *font, const char *cstr);
FONT_DEF void font_render(Font *font, const char* data, u32 data_len,
			  u32 *pixels, u32 width, u32 height,
			  u32 x, u32 y,
			  u32 color);
FONT_DEF void font_free(Font *font);

#endif

typedef struct{
  u8 *data;          //data of glyphs
  int *xs, *ys, *ws; //glyph x-/y-offset + glyph width
  unsigned int width, height;
}Font2;

/* FONT_DEF bool font_init2(Font2 *font, const char* font_path, int font_height); */
/* FONT_DEF bool font_init_from2(Font2 *font, char *ttf_buffer, int font_height); */
FONT_DEF unsigned int font_estimate_width2(const Font2 *font, const char *cstr);
FONT_DEF void font_render2(Font2 *font, const char* data, u32 data_len,
			  u32 *pixels, u32 width, u32 height,
			  u32 x, u32 y,
			  u32 color);
FONT_DEF void font_free2(Font2 *font);

#ifdef FONT_IMPLEMENTATION

#ifdef STB_TRUETYPE_IMPLEMENTATION

FONT_DEF bool font_init2(Font2 *font, const char* font_path, int font_height);
FONT_DEF bool font_init_from2(Font2 *font, char *ttf_buffer, int font_height);

#if 0

//TODO: unhardcode 512x512 bitmap
FONT_DEF bool font_init(Font *font, const char* font_path, int font_height) {
  if(!font) {
    return false;
  }

  char *content = font_slurp_file(font_path, NULL);
  if(!content) {
    return false;
  }  

  char *memory = (char *) malloc(512 * 512 + sizeof(font_stbtt_bakedchar) * 96);
  if(!memory) {
    free(content);
    return false;
  }
  
  font->height = font_height;
  font->bitmap = (unsigned char * ) memory;
  font->cdata = (font_stbtt_bakedchar *) (memory + 512 * 512);
  
  int ret = stbtt_BakeFontBitmap((unsigned char *) content, 0, (float) font_height, font->bitmap, 512, 512, 32, 96,
				 (stbtt_bakedchar *) font->cdata);
  if(ret <= 0) {
    free(content);
    return false;
  }

  free(content);
  return true;
}

FONT_DEF void font_render(Font *font, const char* data, u32 data_len,
			  u32 *pixels, u32 _width2, u32 _height2,
			  u32 _x0, u32 _y0,
			  u32 color) {
  stbtt_aligned_quad q;
  float x0=(float) _x0;
  float y0=(float) _y0;

  s32 width2 = (s32) _width2;
  s32 height2 = (s32) _height2;

  u32 alpha = 0xff000000 & color;
  color = 0x00ffffff & color;

  for(u32 i=0;i<data_len;i++) {

    char c = data[i];
    
    if(c >= 32 && c < 128) { 
      stbtt_GetBakedQuad((stbtt_bakedchar *) font->cdata, 512, 512, c-32, &x0, &y0, &q, 1);//1=opengl & d3d10+,0=d3d9
      s32 q_x0 = (s32) q.x0;
      s32 q_y0 = (s32) q.y0;

      s32 width = (s32) (q.s1 * 512 - q.s0 * 512);
      s32 height = (s32) (q.t1 * 512 - q.t0 * 512);
  
      u8 *font_data = &font->bitmap[(s32) (q.t0 * 512 * 512 + q.s0 * 512)];
      for(s32 y=0;y<height;y++) {
	u8 *pixel = font_data;
	for(s32 x=0;x<width;x++) {
	  u8 d = *pixel++;
	  
	  s32 nx = x + q_x0;
	  s32 ny = 2*_y0 - y - q_y0;
	  if(nx<0 || nx>=width2 || ny<0 || ny>=height2) {
	    continue;
	  }

	  u32 old_color = pixels[ny*width2+nx];
	  u32 new_color = rgba_mix( ((d << 24) | color), old_color) & 0x00ffffff;
	  pixels[ny*width2+nx] = rgba_mix( (alpha | new_color), old_color);
	}
	font_data += 512;
      }
    }
  }

}

FONT_DEF unsigned int font_estimate_width(const Font *font, const char *cstr) {

  stbtt_aligned_quad q;
  float x0= 0.f;
  float y0= 0.f;

  while(*cstr) {
    stbtt_GetBakedQuad((stbtt_bakedchar *) font->cdata, 512, 512, *cstr-32, &x0, &y0, &q, 1);
    ++cstr;
  }

  return (unsigned int) (x0);
}

FONT_DEF void font_free(Font *font) {
  if(!font) {
    return;
  }

  free(font->bitmap);
}

#endif

FONT_DEF bool font_init_from2(Font2 *font, char *ttf_buffer, int font_height) {
  if(!font) {
    return false;
  }  
  unsigned int ascii_vals = 127 - 32;
  unsigned int ascii_width = ascii_vals * font_height;
  //data: ascii_vals * font_height * font_height * sizeof(u8)
  //xs, ys, ws: 3 * acsii_vals * sizeof(int)

  u8 *data = malloc(ascii_vals * font_height * font_height * sizeof(u8) + // font->data
		    3 * ascii_vals * sizeof(unsigned int)); // font->xs, font->ys, font->ws
  if(!data) {
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
  stbtt_InitFont(&font_info, (u8 *) ttf_buffer, 0);
  
  float scale = stbtt_ScaleForPixelHeight(&font_info, (float) font_height);
  int _font_height = (int) font_height;

  for(int c=32;c<=126;c++) {
    unsigned int glyph_off = (c-32)*font_height;
    
    int w, h, x ,y;
    u8 *bitmap =
      stbtt_GetCodepointSDF(&font_info, scale, c, 0, 128, 16.f, &w, &h, &x, &y);
      //stbtt_GetCodepointBitmap(&font_info, 0, scale, c, &w, &h, &x, &y);

    font->xs[c - 32] = x;
    font->ys[c - 32] = y;
    font->ws[c - 32] = w;
    
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
  
  return true;
  
}

FONT_DEF bool font_init2(Font2 *font, const char* font_path, int font_height) {
  if(!font) {
    return false;
  }

  int padding = 1;
  font_height += padding;

  char *content;
  size_t content_size;
  if(!io_slurp_file(font_path, &content, &content_size)) {
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
  
  float scale = stbtt_ScaleForPixelHeight(&font_info, (float) (font_height + padding) );
  int _font_height = (int) font_height;

  for(int c=32;c<=126;c++) {
    unsigned int glyph_off = (c-32)*font_height;
    
    int w, h, x ,y;
    u8 *bitmap =
	stbtt_GetCodepointSDF(&font_info, scale, c, padding, 128, 64.0, &w, &h, &x, &y);
    //stbtt_GetCodepointBitmap(&font_info, 0, scale, c, &w, &h, &x, &y);

    font->xs[c - 32] = x;
    font->ys[c - 32] = y;
    font->ws[c - 32] = w;
    
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

FONT_DEF unsigned int font_estimate_width2(const Font2 *font, const char *cstr) {
  unsigned int x0 = 0;
  while(*cstr) {
      char c = *cstr++;
      if(c < 32  || c == ' ') { // c <= 128
      x0 += font->height * 1 / 4;
    }
    else x0 += font->xs[c - 32] + font->ws[c - 32];
  }
  return x0;
}

FONT_DEF void font_render2(Font2 *font, const char* data, u32 data_len,
			  u32 *pixels, u32 width0, u32 height0,
			  u32 x0, u32 y0,
			  u32 color) {
    s32 width = (s32) width0;
    s32 height = (s32) height0;

    s32 x_off = (s32) x0;
    s32 y_off = (s32) y0;

    u32 alpha = 0xff000000 & color;
    color = 0x00ffffff & color;

    for(u32 k=0;k<data_len;k++) {
	char c = data[k];
	
	if(c == ' ') {
	    x_off += font->height / 2;
	    continue;
	}

	if(c < 32 || c >= 127) {
	  c = '?';
	}

	for(u32 j=0;j<font->height;j++) {
	    for(s32 i=0;i<font->ws[c - 32];i++) {
		s32 y = y_off + j + font->ys[c - 32];
		s32 x = x_off + i + font->xs[c - 32];

		y = 2*y_off - y;
	  
		if(x<0 || x>=width || y<0 || y>=height) {
		    continue;
		}
		char d = font->data[(j * font->width) + ((c - 32) * font->height) + i];
		u32 old_color = pixels[y*width+x];
		u32 new_color = rgba_mix( ((d << 24) | color), old_color) & 0x00ffffff;
		pixels[y*width+x] = rgba_mix( (alpha | new_color), old_color);
	    }
	}

	x_off += font->ws[c - 32] + font->xs[c - 32];

    }
}

FONT_DEF void font_free2(Font2 *font) {
  if(!font) {
    return;
  }

  free(font->data);
}

#endif //FONT_IMPLEMENTATION

#endif //FONT_H_
