#ifndef YUV4MPEG2_H_
#define YUV4MPEG2_H_

void rgb_to_ycbcr(unsigned char _b, unsigned char _g, unsigned char _r,
		  unsigned char *y, unsigned char *cb, unsigned char *cr) {
  float r = (float) _r;
  float g = (float) _g;
  float b = (float) _b;
  *y  = (unsigned char) (16.f + 65.738f*r/256.f + 129.057f*g/256.f + 25.064f*b/256.f);
  *cb = (unsigned char) (128.f - 37.945f*r/256.f - 74.494f*g/256.f + 112.439f*b/256.f);
  *cr = (unsigned char) (128.f + 112.439f*r/256.f - 94.154f*g/256.f - 18.285f*b/256.f);
}

void writeHeader(FILE *f, size_t width, size_t height, size_t fps) {
  fprintf(f, "YUV4MPEG2 W%ld H%ld F%ld:1 Ip A1:1 C444\n", width, height, fps);
}

void writeFrame(FILE *f, uint32_t* src_data, size_t len, size_t stride) {
  unsigned char r, g, b;
  unsigned char y, cb, cr;
  size_t width = stride;
  size_t height = len/width;
  
  fprintf(f, "FRAME\n");
  for(size_t dy=0;dy<height;dy++) {
    for(size_t dx=0;dx<width;dx++) {
      size_t p = dy*width+dx;
      b = (src_data[p] & 0xff);
      g = ((src_data[p] & 0xff00) >> 8);
      r = ((src_data[p] & 0xff0000) >> 16);

      rgb_to_ycbcr(r, g, b, &y, &cb, &cr);
      fwrite(&y, 1, 1, f);
    }
  }
  for(size_t dy=0;dy<height;dy++) {
    for(size_t dx=0;dx<width;dx++) {
      size_t p = dy*width+dx;
      b = (src_data[p] & 0xff);
      g = ((src_data[p] & 0xff00) >> 8);
      r = ((src_data[p] & 0xff0000) >> 16);

      rgb_to_ycbcr(r, g, b, &y, &cb, &cr);
      fwrite(&cb, 1, 1, f);
    }
  }
  for(size_t dy=0;dy<height;dy++) {
    for(size_t dx=0;dx<width;dx++) {
      size_t p = dy*width+dx;
      b = (src_data[p] & 0xff);
      g = ((src_data[p] & 0xff00) >> 8);
      r = ((src_data[p] & 0xff0000) >> 16);

      rgb_to_ycbcr(r, g, b, &y, &cb, &cr);
      fwrite(&cr, 1, 1, f);
    }
  }
}

#endif //YUV4MPEG2_H_
