#ifndef PEN_H_
#define PEN_H_

void fill(uint32_t* src, uint32_t width, uint32_t height, uint32_t color);
void copy(uint32_t* src, uint32_t width, uint32_t height,
	  uint32_t* src2, uint32_t width2, uint32_t height2,
	  int x0, int y0, int w, int h);
void rect(uint32_t* src, uint32_t height, uint32_t width,
	  int x0, int y0, int w, int h, int color);
void circle(uint32_t* src, uint32_t width, uint32_t height,
	    int cx, int cy, int _r, int color);
void triangle(uint32_t* src, uint32_t width, uint32_t height,
	      int x1, int y1, int x2, int y2, int x3, int y3, int color);

#ifdef PEN_IMPLEMENTATION

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

      if(0<=x && x<(int) width && 0<=y && y<(int) height) {
	src[y*width+x] = src2[ny*width2+nx];
      }      
    }
  }
}

void rect(uint32_t* src, uint32_t height, uint32_t width,
	  int x0, int y0, int w, int h, int color) {
  for(int dx = 0; dx < w; ++dx) {
    for(int dy = 0; dy < h; ++dy) {
      int x = x0 + dx;
      int y = y0 + dy;
      if(0<=x && x<(int) width && 0<=y && y<(int) height) {
	src[y*width+x] = color;
      }
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

void swap(int *a, int *b) {
  int t = *a;
  *a = *b;
  *b = t;
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
