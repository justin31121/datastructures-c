#ifndef RENDER_H_H
#define RENDER_H_H

#ifdef RENDERER_IMPLEMENTATION
#define VEC_IMPLEMENTATION
#endif //RENDERER_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "vec.h"

#ifndef RENDER_DEF
#define RENDER_DEF static inline
#endif //RENDER_DEF

typedef struct{
  Vec2f position;
  Vec4f color;
  Vec2f uv;
}Renderer_Vertex;

typedef enum {
	      RENDERER_VERTEX_ATTR_POSITION = 0,
	      RENDERER_VERTEX_ATTR_COLOR,
	      RENDERER_VERTEX_ATTR_UV,
	      COUNT_OPEN_VERTEX_ATTRS
}Renderer_Vertex_Attribute;

typedef enum{
	     SHADER_FOR_COLOR = 0,
	     SHADER_FOR_IMAGE,
	     SHADER_FOR_RIPPLE,
	     SHADER_FOR_TEXT,
	     COUNT_SHADERS
}Renderer_Shader;

/*
typedef struct{
  int width, height;
  char *data;
  bool grey;
}Renderer_Texture;
*/

#define RENDERER_VERTEX_CAP (1024 * 10)

typedef struct{
  GLuint vao;
  GLuint vbo;
  GLuint programs[COUNT_SHADERS];

  Renderer_Vertex verticies[RENDERER_VERTEX_CAP];
  unsigned int verticies_count;
  
  GLuint textures;
  unsigned int images_count;
  unsigned int image;
  float time;

  Vec2f resolution;
}Renderer;

RENDER_DEF bool renderer_init(Renderer *renderer);
RENDER_DEF void renderer_triangle(Renderer *renderer,
				  Vec2f p1, Vec2f p2, Vec2f p3,
				  Vec4f c1, Vec4f c2, Vec4f c3,
				  Vec2f uv1, Vec2f uv2, Vec2f uv3);
RENDER_DEF void renderer_solid_triangle(Renderer *renderer,
					Vec2f p1, Vec2f p2, Vec2f p3, Vec4f color);
RENDER_DEF void renderer_solid_triangle3(Renderer *renderer,
					 Vec2f p1, Vec2f p2, Vec2f p3, Vec4f c1, Vec4f c2, Vec4f c3);
RENDER_DEF void renderer_solid_rect(Renderer *renderer, Vec2f pos, Vec2f size, Vec4f color);
RENDER_DEF void renderer_image_rect(Renderer *renderer, Vec2f p, Vec2f s, Vec2f uvp, Vec2f usvs);
RENDER_DEF void renderer_image_rect_alpha(Renderer *r, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, float alpha);
RENDER_DEF unsigned int renderer_push_texture(Renderer *renderer, int width, int height, char *data, bool grey);
RENDER_DEF void renderer_text(Renderer *renderer, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, Vec4f c);
RENDER_DEF void renderer_set_shader(Renderer *renderer, Renderer_Shader shader);
RENDER_DEF void renderer_flush(Renderer *renderer);

#ifdef RENDERER_IMPLEMENTATION

static const char* renderer_vertex_shader_source =
  "#version 330 core\n"
  "\n"
  "\n"
  "layout(location = 0) in vec2 position;\n"
  "layout(location = 1) in vec4 color;\n"
  "layout(location = 2) in vec2 uv;\n"
  "\n"
  "uniform float resolution_x;\n"
  "uniform float resolution_y;\n"
  "\n"
  "out vec4 out_color;\n"
  "out vec2 out_uv;\n"
  "\n"
  "vec2 resolution_project(vec2 point) {\n"
  "      return 2 * point / vec2(resolution_x, resolution_y) - 1;\n"
  "}\n"
  "\n"
  "void main() {\n"
  "  gl_Position = vec4(resolution_project(position), 0, 1);\n"
  "  out_color = color;\n"
  "  out_uv = uv;\n"
  "}";
  

static_assert(COUNT_SHADERS == 4, "The amount of Shaders has changed.");
static const char *renderer_frag_shader_sources[] = {
    [SHADER_FOR_COLOR] =
    "#version 330 core\n"
    "\n"
    "in vec4 out_color;\n"
    "\n"
    "void main() {\n"
    "  gl_FragColor = out_color;\n"
    "}\n",  
    [SHADER_FOR_IMAGE] =
    "#version 330 core\n"
    "\n"
    "uniform sampler2D image;\n"
    "\n"
    "in vec4 out_color;\n"
    "in vec2 out_uv;\n"
    "\n"
    "void main() {\n"
    "  vec4 color = texture(image, vec2(out_uv.x, 1-out_uv.y));\n"
    "  color.w = color.w * out_color.w;\n"
    "  gl_FragColor = color;\n"
    "}\n",
    [SHADER_FOR_RIPPLE] = //http://adrianboeing.blogspot.com/2011/02/ripple-effect-in-webgl.html
    "#version 330 core\n"
    "\n"
    "#ifdef GL_ES\n"
    "precision highp float;\n"
    "#endif"
    "\n"
    "uniform float time;\n"
    "uniform sampler2D image;\n"
    "\n"
    "in vec4 out_color;\n"
    "in vec2 out_uv;\n"
    "\n"
    "void main() {\n"
    "  vec2 cPos = 2 * out_uv - 1;\n"
    "  float cLength = length(cPos);\n"
    "  vec2 img_uv = out_uv + (cPos/cLength)*mix(cos(cLength*12-time*4)*0.03, 0, cLength / 0.25);\n"
    "\n"
    "gl_FragColor = texture(image, vec2(img_uv.x, 1 - img_uv.y));\n"
    "}\n",
    [SHADER_FOR_TEXT] =
    "#version 330 core\n"
    "\n"
    "uniform sampler2D image;\n"
    "\n"
    "in vec4 out_color;\n"
    "in vec2 out_uv;\n"
    "\n"
    "void main() {\n"
    "  float d = texture(image, vec2(out_uv.x, 1 - out_uv.y)).r;\n"
    "  float aaf = fwidth(d);\n"
    "  float alpha = smoothstep(0.5 - aaf, 0.5 + aaf, d);\n"
    "  gl_FragColor = vec4(out_color.rgb, alpha * out_color.a);\n"
   "}\n"
  };

RENDER_DEF const char *shader_type_as_cstr(GLuint shader)
{
    switch (shader) {
    case GL_VERTEX_SHADER:
        return "GL_VERTEX_SHADER";
    case GL_FRAGMENT_SHADER:
        return "GL_FRAGMENT_SHADER";
    default:
        return "(Unknown)";
    }
}

RENDER_DEF bool renderer_compile_shader(GLuint *shader, GLenum shaderType, const char *shader_source) { 
  *shader  =glCreateShader(shaderType);
  glShaderSource(*shader, 1, &shader_source, NULL);
  glCompileShader(*shader);

  GLint compiled = 0;
  glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

  if (!compiled) {
    GLchar message[1024];
    GLsizei message_size = 0;
    glGetShaderInfoLog(*shader, sizeof(message), &message_size, message);
    fprintf(stderr, "ERROR: could not compile %s\n", shader_type_as_cstr(shaderType));
    fprintf(stderr, "%.*s\n", message_size, message);
    return false;
  }

  return true;
}

RENDER_DEF bool renderer_link_program(GLuint *program, GLuint vertex_shader, GLuint fragment_shader) { 
  *program = glCreateProgram();
  glAttachShader(*program, vertex_shader);
  glAttachShader(*program, fragment_shader);

  glLinkProgram(*program);
  
  GLint linked = 0;
  glGetProgramiv(*program, GL_LINK_STATUS, &linked);
  if(!linked) {
    GLsizei message_size = 0;
    GLchar message[1024];

    glGetProgramInfoLog(*program, sizeof(message), &message_size, message);
    fprintf(stderr, "ERROR: Program Linking: %.*s\n", message_size, message);
  }
  
  return true;
}

RENDER_DEF bool renderer_init(Renderer *r) {
  r->verticies_count = 0;

  glGenVertexArrays(1, &r->vao);
  glBindVertexArray(r->vao);

  glGenBuffers(1, &r->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(r->verticies), r->verticies, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(RENDERER_VERTEX_ATTR_POSITION);
  glVertexAttribPointer(RENDERER_VERTEX_ATTR_POSITION,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(Renderer_Vertex),
			(GLvoid *) offsetof(Renderer_Vertex, position));

  glEnableVertexAttribArray(RENDERER_VERTEX_ATTR_COLOR);
  glVertexAttribPointer(RENDERER_VERTEX_ATTR_COLOR,
			4,
			GL_FLOAT,
			GL_FALSE,
			sizeof(Renderer_Vertex),
			(GLvoid *) offsetof(Renderer_Vertex, color));
  
  glEnableVertexAttribArray(RENDERER_VERTEX_ATTR_UV);
  glVertexAttribPointer(RENDERER_VERTEX_ATTR_UV,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(Renderer_Vertex),
			(GLvoid *) offsetof(Renderer_Vertex, uv));

  GLuint shaders[2];

  if(!renderer_compile_shader(&shaders[0], GL_VERTEX_SHADER, renderer_vertex_shader_source)) {
    return false;
  }

  for(Renderer_Shader shader = 0; shader < COUNT_SHADERS; shader++) {
    if(renderer_frag_shader_sources[shader] == NULL) {
      continue;
    }
    if(!renderer_compile_shader(&shaders[1], GL_FRAGMENT_SHADER, renderer_frag_shader_sources[shader])) {
	return false;
    }
    if(!renderer_link_program(&r->programs[shader], shaders[0], shaders[1])) {
      return false;
    }
  }
  
  return true;
}

RENDER_DEF void renderer_vertex(Renderer *r, Vec2f p, Vec4f c, Vec2f uv) {
  assert(r->verticies_count < RENDERER_VERTEX_CAP);
  Renderer_Vertex *last = &r->verticies[r->verticies_count];
  last->position = p;
  last->color = c;
  last->uv = uv;
  r->verticies_count++;
}

RENDER_DEF void renderer_triangle(Renderer *r,
				  Vec2f p1, Vec2f p2, Vec2f p3,
				  Vec4f c1, Vec4f c2, Vec4f c3,
				  Vec2f uv1, Vec2f uv2, Vec2f uv3) {
  renderer_vertex(r, p1, c1, uv1);
  renderer_vertex(r, p2, c2, uv2);
  renderer_vertex(r, p3, c3, uv3);
}

RENDER_DEF void renderer_solid_triangle3(Renderer *r,
					Vec2f p1, Vec2f p2, Vec2f p3,
					 Vec4f c1, Vec4f c2, Vec4f c3) {
  Vec2f uv = vec2f(1, 1);
  renderer_triangle(r, p1, p2, p3, c1, c2, c3, uv, uv, uv);
}

RENDER_DEF void renderer_solid_triangle(Renderer *r, Vec2f p1, Vec2f p2, Vec2f p3, Vec4f color) {
  Vec2f uv = vec2f(1, 1);
  renderer_triangle(r, p1, p2, p3, color, color, color, uv, uv, uv);
}

RENDER_DEF void renderer_quad(Renderer *r, Vec2f p1, Vec2f p2, Vec2f p3, Vec2f p4, Vec4f c1, Vec4f c2, Vec4f c3, Vec4f c4, Vec2f uv1, Vec2f uv2, Vec2f uv3, Vec2f uv4) {
  renderer_triangle(r, p1, p2, p4, c1, c2, c4, uv1, uv2, uv4);
  renderer_triangle(r, p1, p3, p4, c1, c3, c4, uv1, uv3, uv4);
}

RENDER_DEF void renderer_solid_rect(Renderer *r, Vec2f pos, Vec2f size, Vec4f color) {
  Vec2f uv = vec2f(0, 0);
  renderer_quad(r, pos,
		vec2f_add(pos, vec2f(size.x, 0)),
		vec2f_add(pos, vec2f(0, size.y)),
		vec2f_add(pos, size),
		color, color, color, color, uv, uv, uv, uv);
}

RENDER_DEF void renderer_image_rect(Renderer *r, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs)
{
  Vec4f c = vec4f(1, 1, 1, 1);
  renderer_quad(
		r,
		p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
		c, c, c, c,
		uvp, vec2f_add(uvp, vec2f(uvs.x, 0)), vec2f_add(uvp, vec2f(0, uvs.y)), vec2f_add(uvp, uvs));
}

RENDER_DEF void renderer_image_rect_alpha(Renderer *r, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, float alpha)
{
  Vec4f c = vec4f(1, 1, 1, alpha);
  renderer_quad(
		r,
		p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
		c, c, c, c,
		uvp, vec2f_add(uvp, vec2f(uvs.x, 0)), vec2f_add(uvp, vec2f(0, uvs.y)), vec2f_add(uvp, uvs));
}

RENDER_DEF void renderer_text(Renderer *r, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, Vec4f c) {
  renderer_quad(
        r,
        p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
        c, c, c, c,
        uvp, vec2f_add(uvp, vec2f(uvs.x, 0)), vec2f_add(uvp, vec2f(0, uvs.y)), vec2f_add(uvp, uvs));  
}

static GLenum renderer_index_to_texture(unsigned int index) {
  switch(index) {
  case 0:
    return GL_TEXTURE0;
  case 1:
    return GL_TEXTURE1;
  case 2:
    return GL_TEXTURE2;
  case 3:
    return GL_TEXTURE3;
  default:
    fprintf(stderr, "ERROR: You uplaoded too many textures, or provided a bad value to open_renderer.image");
    exit(1);
  }
  fprintf(stderr, "ERROR: unreachable");
  exit(1);
}

RENDER_DEF unsigned int renderer_push_texture(Renderer *r, int width, int height, char *data, bool grey) {
  glActiveTexture(renderer_index_to_texture(r->images_count));

  
  glGenTextures(1, &r->textures);
  glBindTexture(GL_TEXTURE_2D, r->textures);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if(grey) {
    glTexImage2D(GL_TEXTURE_2D,
		 0,
		 GL_RED,
		 width,
		 height,
		 0,
		 GL_RED,
		 GL_UNSIGNED_BYTE,
		 data);
  } else {
    glTexImage2D(GL_TEXTURE_2D,
		 0,
		 GL_RGBA,
		 width,
		 height,
		 0,
		 GL_RGBA,
		 GL_UNSIGNED_INT_8_8_8_8_REV,
		 data);
  }


  return r->images_count++;
}

RENDER_DEF void renderer_set_shader(Renderer *r, Renderer_Shader shader) {
  glViewport(0, 0, r->resolution.x, r->resolution.y);

  glUseProgram(r->programs[shader]);
  glUniform1fv(glGetUniformLocation(r->programs[shader], "resolution_x"), 1, (float *) &r->resolution);
  glUniform1fv(glGetUniformLocation(r->programs[shader], "resolution_y"), 1, ((float *) &r->resolution) + 1);

  GLint uniformLocation =  glGetUniformLocation(r->programs[shader], "image");
  glUniform1i(uniformLocation, (int) r->image);

  glUniform1fv(glGetUniformLocation(r->programs[shader], "time"), 1, &r->time);
}

RENDER_DEF void renderer_flush(Renderer *r) {
  glBufferSubData(GL_ARRAY_BUFFER, 0, ((int) r->verticies_count * (int) sizeof(Renderer_Vertex)), r->verticies);
  glDrawArrays(GL_TRIANGLES, 0, (int) r->verticies_count);
  r->verticies_count = 0;
}


#endif //RENDERER_IMPLEMENTATION

#endif //RENDER_H_H
