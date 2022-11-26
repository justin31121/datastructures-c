#include <stdint.h>

#define PEN_IMPLEMENTATION
#include "pen.h"

#define ARRAY_IMPLEMENTATION
#include "array.h"

#define WIDTH 800
#define HEIGHT 600

typedef struct{
  int x, y;
}Point;

Arr *points;

void init() {
  points = arr_init(sizeof(Point));
  Point p;
  for(int i=0;i<10;i++) {
    p.x = 30;
    p.y = WIDTH * i / 10;
    arr_push(points, &p);
  }

  printf("%lld\n", points->count);
}

uint32_t pixels[WIDTH*HEIGHT];

uint32_t *render() {
  fill(pixels, WIDTH, HEIGHT, 0xff181818);
  for(size_t i=0;i<points->count;i++) {
    Point *p = arr_get(points, i);
    circle(pixels, WIDTH, HEIGHT,
	    p->x, p->y, 10, 0xff33ddff);
  }
  return pixels;
}

#include "serve.c"
