#ifndef PEN_H_
#define PEN_H_

#include <stdint.h>

void circle(uint32_t* src, uint32_t width, uint32_t height, int cx, int cy, int _r, int color);
void rect(uint32_t* src, uint32_t width, uint32_t height, int x0, int y0, int w, int h, int color);
void line(uint32_t *src, uint32_t width, uint32_t height, int x1, int y1, int x2, int y2, uint32_t color);
void copy(uint32_t* src, uint32_t width, uint32_t height, uint32_t* src2, uint32_t width2, uint32_t height2, int x0, int y0, int w, int h);
void copy2(uint32_t* src, uint32_t width, uint32_t height, uint32_t* src2, uint32_t width2, uint32_t height2, uint32_t stride_in_bytes, int x0, int y0, int w, int h);
void fill(uint32_t* src, uint32_t width, uint32_t height, uint32_t color);
void triangle(uint32_t* src, uint32_t width, uint32_t height, int x1, int y1, int x2, int y2, int x3, int y3, int color);
void project(uint32_t* src, int width, int height, int x0, int y0, int w, int h, int f(int), int px0, int px1, int py0, int py1, uint32_t color);

#ifdef PEN_IMPLEMENTATION

static inline uint32_t rgba_mix(uint32_t color, uint32_t base) {

  uint8_t color_alpha = (0xff000000 & color) >> 24;

  uint8_t base_red = 0xff & base;
  uint8_t red = color_alpha * ((0xff & color) - base_red) / 0xff + base_red;
  
  uint8_t base_green = (0xff00 & base) >> 8;
  uint8_t green = color_alpha * (((0xff00 & color) >> 8) - base_green) / 0xff + base_green;
  
  uint8_t base_blue = (0xff0000 & base) >> 16;
  uint16_t blue = color_alpha * (((0xff0000 & color) >> 16) - base_blue) / 0xff + base_blue;
  
  return (color_alpha << 24) | (blue << 16) | (green << 8) | red;
}

void fill(uint32_t* src, uint32_t width, uint32_t height, uint32_t color) {
  for(uint32_t i=0;i<width*height;i++) {
    src[i] = color;
  }
}

void copy(uint32_t* src, uint32_t width, uint32_t height,
	  uint32_t* src2, uint32_t width2, uint32_t height2,
	  int x0, int y0, int w, int h) {
  for(int dy = 0; dy<h; ++dy) {
    for(int dx = 0; dx<w; ++dx) {
      int x = x0 + dx;
      int y = y0 + dy;
      int nx = dx*width2/w;
      int ny = dy*height2/h;


      if(x<0 || x>=(int) width || y<0 || y>=(int) height) {
	continue;
      }      
      src[y*width+x] = rgba_mix(src2[ny*width2+nx], src[y*width+x]);
    }
  }
}

void copy2(uint32_t* src, uint32_t width, uint32_t height, uint32_t* src2, uint32_t width2, uint32_t height2, uint32_t stride_in_bytes, int x0, int y0, int w, int h) {
  (void) width2;
  for(int dy = 0; dy<h; ++dy) {
    for(int dx = 0; dx<w; ++dx) {
      int x = x0 + dx;
      int y = y0 + dy;
      int nx = dx*stride_in_bytes/w;
      int ny = dy*height2/h;

      if(0<=x && x<(int) width && 0<=y && y<(int) height) {
	src[y*width+x] = src2[ny*(stride_in_bytes)+nx];
      }      
    }

  }
}

void swap(int *a, int *b) {
  int t = *a;
  *a = *b;
  *b = t;
}

void project(uint32_t* src, int width, int height,
	     int x0, int y0, int w, int h,
	     int f(int), int px0, int px1, int py0, int py1, uint32_t color) {
  int dpx = px1 - px0;
  int dpy = py1 - py0;
  
  for(int dx = 0; dx < w ; dx++) {
    int x1 = (px0 + dx*dpx/w);
    int x2 = (px0 + (dx+1)*dpx/w);
    int y1 = f(x1);
    int y2 = f(x2);
    if(y1 > y2) swap(&y1, &y2);


    int sx = x0 + dx;

    for(int y=y1;y<=y2;y++) {
      if(y<py0 || py1<y) continue;
      int sy = height - (y0 + (y - py0)*h/dpy);
      if(0<=sx && sx<width && 0<=sy && sy<height) {
	src[sy*width+sx] = color;
      }
    }
  }
}

void line(uint32_t *src, uint32_t width, uint32_t height, int x1, int y1, int x2, int y2, uint32_t color) {
  int dx = x2 - x1;
  int dy = y2 - y1;

  // If both of the differences are 0 there will be a division by 0 below.
  if (dx == 0 && dy == 0) {
    if (0 <= x1 && x1 < (int) width && 0 <= y1 && y1 < (int) height) {
      src[y1*width+x1] = color; 
    }
    return;
  }

  int dx_abs = dx < 0 ? dx * -1 : dx;
  int dy_abs = dy < 0 ? dy * -1 : dy;
  if (dx_abs > dy_abs) {
    if (x1 > x2) {
      swap(&x1, &x2);
      swap(&y1, &y2);
    }

    // Cull out invisible line
    if (x1 > (int) width) return;
    if (x2 < 0) return;

    // Clamp the line to the boundaries
    if (x1 < 0) x1 = 0;
    if (x2 >= (int) width) x2 = (int) width - 1;

    for (int x = x1; x <= x2; ++x) {
      int y = dy*(x - x1)/dx + y1;
      if (0 <= y && y < (int) height) {
	src[y*width+x] = color; 
      }
    }
  } else {
    if (y1 > y2) {
      swap(&x1, &x2);
      swap(&y1, &y2);
    }

    if (y1 > (int) height) return;
    if (y2 < 0) return;

    if (y1 < 0) y1 = 0;
    if (y2 >= (int) height) y2 = (int) height - 1;

    for (int y = y1; y <= y2; ++y) {
      int x = dx*(y - y1)/dy + x1;
      if (0 <= x && x < (int) width) {
	src[y*width+x] = color; 
      }
    }
  }
}

void rect(uint32_t* src, uint32_t width, uint32_t height,
	  int x0, int y0, int w, int h, int color) {
  for(int dy = 0; dy < h; ++dy) {
    for(int dx = 0; dx < w; ++dx) {
      int x = x0 + dx;
      int y = y0 + dy;
      if(x<0 || x>=((int) width) || y<0 || y>=((int) height)) {
	continue;
      }
      src[y*width+x] = rgba_mix(color, src[y*width+x]);
    }
  } 
}

void circle(uint32_t* src, uint32_t width, uint32_t height,
	    int cx, int cy, int _r, int color) {
  int r = (int) _r;
  for(int dx = -r; dx < r; ++dx) {
    for(int dy = -r; dy < r; ++dy) {
      int x = cx + dx;
      int y = cy + dy;
      if(dx*dx+dy*dy>=r*r) continue;
      if(0<=x && x<(int) width && 0<=y && y<(int) height) {
	src[y*width+x] = color;
      }
    }
  }
}

void sort_by_y(int *x1, int *y1, int *x2, int *y2, int *x3, int *y3) {
  if(*y1 > *y2) {
    swap(y1, y2);
    swap(x1, x2);
  }

  if(*y2 > *y3) {
    swap(y2, y3);
    swap(x2, x3);
  }

  if(*y1 > *y2) {
    swap(y1, y2);
    swap(x1, x2);
  }
}

typedef struct{
  int dx;
  int dy;
  int x1;
  int y1;
}Line;

#define line_slope(l) l.dy/l.dx
#define line_off(l) l.y1 - l.dy*l.x1/l.dx
#define line_y(l, x) l.y1 + (x - l.x1)*l.dy/l.dx
#define line_x(l, y) (y*l.dx - l.y1*l.dx + l.dy*l.x1)/l.dy

Line make_line(int x1, int y1, int x2, int y2) {
  return (Line) {(x2-x1), (y2-y1), x2, y2};
}

void triangle(uint32_t* src, uint32_t width, uint32_t height,
	      int x1, int y1, int x2, int y2, int x3, int y3, int color) {
  sort_by_y(&x1, &y1, &x2, &y2, &x3, &y3);
  
  //UPPER
  Line line13 = make_line(x1, y1, x3, y3);  
  Line line12 = make_line(x1, y1, x2, y2);
  for(int y=y1;y<y2;y++) {
    int px1 = line_x(line12, y);
    int px2 = line_x(line13, y);
    if(px1 > px2) swap(&px1, &px2);
    for(int x=px1;x<px2;x++) {
      if(0<=x && x<(int) width && 0<=y && y<(int) height) {
	src[y*width+x] = color;
      }      
    }
  }

  //LOWER
  Line line23 = make_line(x2, y2, x3, y3);
  for(int y=y2;y<y3;y++) {
    int px1 = line_x(line23, y);
    int px2 = line_x(line13, y);
    if(px1 > px2) swap(&px1, &px2);
    for(int x=px1;x<px2;x++) {
      if(0<=x && x<(int) width && 0<=y && y<(int) height) {
      	src[y*width+x] = color;
      }      
    }
  }
}

#endif //PEN_IMPLEMENTATION

#endif //PEN_H_
