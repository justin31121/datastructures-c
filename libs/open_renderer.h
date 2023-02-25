#ifndef OPEN_RENDERER_H_
#define OPEN_RENDERER_H_

#ifdef OPEN_RENDERER_IMPLEMENTATION

#ifndef UTIL_IMPLEMENTATION
#define UTIL_IMPLEMENTATION
#endif //UTIL_IMPLEMENTATION

#endif //OPEN_RENDERER_IMPLEMENTATION
#include "./util.h"

#ifdef OPEN_RENDERER_IMPLEMENTATION

#ifndef VEC_IMPLEMENTATION
#define VEC_IMPLEMENTATION
#endif //VEC_IMPLEMENTATION

#endif //OPEN_RENDERER_IMPLEMENTATION
#include "./vec.h"

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#define OPEN_RENDERER_VERTICIES_CAP 1024

typedef struct{
  Vec2f position;
  Vec4f color;
  Vec2f uv;
}Open_Vertex;

typedef struct{
  int width, height;
  char *data;
  bool grey;
}Open_Texture;

typedef enum {
	      OPEN_VERTEX_ATTR_POSITION = 0,
	      OPEN_VERTEX_ATTR_COLOR,
	      OPEN_VERTEX_ATTR_UV,
	      COUNT_OPEN_VERTEX_ATTRS
}Open_Vertex_Attribute;

typedef enum{
	     SHADER_FOR_COLOR = 0,
	     SHADER_FOR_IMAGE,
	     SHADER_FOR_RIPPLE,
	     SHADER_FOR_TEXT,
	     COUNT_SHADERS
}Open_Shader;

static_assert(COUNT_SHADERS == 4, "The amount of Shaders has changed.");
static const char *open_renderer_vert_shader_file_path = "./simple.vert";
static const char *open_renderer_frag_shader_file_paths[] =
  {
   [SHADER_FOR_COLOR] = "./simple_color.frag",
   [SHADER_FOR_IMAGE] = "./simple_image.frag",
   [SHADER_FOR_RIPPLE] = "./simple_ripple.frag",
   [SHADER_FOR_TEXT] = "./simple_text.frag",
  };

typedef struct{
  GLuint vao;
  GLuint vbo;
  GLuint programs[COUNT_SHADERS];
  GLuint textures;

  Open_Vertex verticies[OPEN_RENDERER_VERTICIES_CAP];
  size_t verticies_count;
  
  size_t images_count;

  Vec2i resolution;
  float time;
  size_t image;
}Open_Renderer;

bool open_renderer_init(Open_Renderer *or);
void open_renderer_set_shader(Open_Renderer *or, Open_Shader shader);
void open_renderer_flush(Open_Renderer *or);
void open_renderer_triangle(Open_Renderer *or, Vec2f p1, Vec2f p2, Vec2f p3, Vec4f c1, Vec4f c2, Vec4f c3, Vec2f uv1, Vec2f uv2, Vec2f uv3);
void open_renderer_quad(Open_Renderer *or, Vec2f p1, Vec2f p2, Vec2f p3, Vec2f p4, Vec4f c1, Vec4f c2, Vec4f c3, Vec4f c4, Vec2f uv1, Vec2f uv2, Vec2f uv3, Vec2f uv4);
void open_renderer_solid_rect(Open_Renderer *or, Vec2f pos, Vec2f size, Vec4f color);
void open_renderer_image_rect(Open_Renderer *or, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs);
void open_renderer_text_rect(Open_Renderer *or, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, Vec4f c);

size_t open_renderer_push_texture(Open_Renderer *or, Open_Texture texture);
void open_renderer_flush_textures(Open_Renderer *or);

#ifdef OPEN_RENDERER_IMPLEMENTATION

static bool compile_shader_file(const char *file_path, GLenum shader_type, GLuint *shader);
static void attach_shaders_to_program(GLuint *shaders, size_t shaders_count, GLuint program);
static bool link_program(GLuint program, const char *file_path, size_t line);
static bool compile_shader_source(const GLchar *source, GLenum shader_type, GLuint *shader);

bool open_renderer_init(Open_Renderer *or) {
  if(!or) {
    return false;
  }

  *or = (Open_Renderer) {0};

  glGenVertexArrays(1, &or->vao);
  glBindVertexArray(or->vao);

  glGenBuffers(1, &or->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, or->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(or->verticies), or->verticies, GL_DYNAMIC_DRAW);

  // position
  glEnableVertexAttribArray(OPEN_VERTEX_ATTR_POSITION);
  glVertexAttribPointer(OPEN_VERTEX_ATTR_POSITION,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(Open_Vertex),
			(GLvoid *) offsetof(Open_Vertex, position));

  // color
  glEnableVertexAttribArray(OPEN_VERTEX_ATTR_COLOR);
  glVertexAttribPointer(OPEN_VERTEX_ATTR_COLOR,
			4,
			GL_FLOAT,
			GL_FALSE,
			sizeof(Open_Vertex),
			(GLvoid *) offsetof(Open_Vertex, color));

  // uv
  glEnableVertexAttribArray(OPEN_VERTEX_ATTR_UV);
  glVertexAttribPointer(OPEN_VERTEX_ATTR_UV,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(Open_Vertex),
			(GLvoid *) offsetof(Open_Vertex, uv));

  GLuint shaders[2] = {0};
  
  if(!compile_shader_file(open_renderer_vert_shader_file_path, GL_VERTEX_SHADER, &shaders[0])) {
    return false;
  }

  for(Open_Shader shader = 0; shader < COUNT_SHADERS; shader++) {
    if (!compile_shader_file(open_renderer_frag_shader_file_paths[shader], GL_FRAGMENT_SHADER, &shaders[1])) {
      return false;
    }
    or->programs[shader] = glCreateProgram();
    attach_shaders_to_program(shaders, sizeof(shaders) / sizeof(shaders[0]), or->programs[shader]);
    if (!link_program(or->programs[shader], __FILE__, __LINE__)) {
      return false;
    }    
  }
  
  return true;
}

static GLenum open_renderer_index_to_texture(size_t index) {
  switch(index) {
  case 0:
    return GL_TEXTURE0;
  case 1:
    return GL_TEXTURE1;
  case 2:
    return GL_TEXTURE2;
  default:
    panic("You uplaoded too many textures, or provided a bad value to open_renderer.image");    
  }
  panic("unreachable");
}

void open_renderer_set_shader(Open_Renderer *or, Open_Shader shader) {
  not_null(or);

  glViewport(0, 0, or->resolution.x, or->resolution.y);
  glUseProgram(or->programs[shader]);

  glUniform2f(glGetUniformLocation(or->programs[shader], "resolution"), (float) or->resolution.x, (float) or->resolution.y);
  glUniform1f(glGetUniformLocation(or->programs[shader], "time"), or->time);
  glUniform1i(glGetUniformLocation(or->programs[shader], "image"), (int) or->image);
  
  /*
    get_uniform_location(programs[SHADER_FOR_COLOR], uniforms);
    glUniform2f(sr->uniforms[UNIFORM_SLOT_RESOLUTION], sr->resolution.x, sr->resolution.y);
    glUniform1f(sr->uniforms[UNIFORM_SLOT_TIME], sr->time);
    glUniform2f(sr->uniforms[UNIFORM_SLOT_CAMERA_POS], sr->camera_pos.x, sr->camera_pos.y);
    glUniform1f(sr->uniforms[UNIFORM_SLOT_CAMERA_SCALE], sr->camera_scale);
  */
}

void open_renderer_flush(Open_Renderer *or) {
  not_null(or);
  
  //--------SYNC
  glBufferSubData(GL_ARRAY_BUFFER, 0, ((int) or->verticies_count * (int) sizeof(Open_Vertex)), or->verticies);
  //--------DRAW
  glDrawArrays(GL_TRIANGLES, 0, (int) or->verticies_count);
  or->verticies_count = 0;
}

static bool open_renderer_render_vertex(Open_Renderer *or, Vec2f p, Vec4f c, Vec2f uv) {
  
  if(or->verticies_count >= OPEN_RENDERER_VERTICIES_CAP) {
    panic("Exceeded Verticies-capacity");
  }
  
  Open_Vertex *last = &or->verticies[or->verticies_count];
  last->position = p;
  last->color = c;
  last->uv = uv;
  
  or->verticies_count++;
  
  return true;
}

void open_renderer_triangle(Open_Renderer *or, Vec2f p1, Vec2f p2, Vec2f p3, Vec4f c1, Vec4f c2, Vec4f c3, Vec2f uv1, Vec2f uv2, Vec2f uv3) {
  not_null(or);

  open_renderer_render_vertex(or, p1, c1, uv1);
  open_renderer_render_vertex(or, p2, c2, uv2);
  open_renderer_render_vertex(or, p3, c3, uv3);
}

//1 - 2
//| \ |
//3 - 4
void open_renderer_quad(Open_Renderer *or, Vec2f p1, Vec2f p2, Vec2f p3, Vec2f p4, Vec4f c1, Vec4f c2, Vec4f c3, Vec4f c4, Vec2f uv1, Vec2f uv2, Vec2f uv3, Vec2f uv4) {
  open_renderer_triangle(or, p1, p2, p4, c1, c2, c4, uv1, uv2, uv4);
  open_renderer_triangle(or, p1, p3, p4, c1, c3, c4, uv1, uv3, uv4);
}

void open_renderer_solid_rect(Open_Renderer *or, Vec2f pos, Vec2f size, Vec4f color) {
  Vec2f uv = vec2fs(0);
  open_renderer_quad(or, pos,
		     vec2f_add(pos, vec2f(size.x, 0)),
		     vec2f_add(pos, vec2f(0, size.y)),
		     vec2f_add(pos, size),
		     color, color, color, color, uv, uv, uv, uv);
}

void open_renderer_image_rect(Open_Renderer *or, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs)
{
    Vec4f c = vec4fs(0);
    open_renderer_quad(
        or,
        p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
        c, c, c, c,
        uvp, vec2f_add(uvp, vec2f(uvs.x, 0)), vec2f_add(uvp, vec2f(0, uvs.y)), vec2f_add(uvp, uvs));
}

void open_renderer_text_rect(Open_Renderer *or, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, Vec4f c) {
  open_renderer_quad(
        or,
        p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
        c, c, c, c,
        uvp, vec2f_add(uvp, vec2f(uvs.x, 0)), vec2f_add(uvp, vec2f(0, uvs.y)), vec2f_add(uvp, uvs));  
}

void open_renderer_ripple_rect(Open_Renderer *or, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs)
{
  Vec4f c = vec4f(uvp.x, uvp.y, uvs.x, uvs.y);
  open_renderer_quad(
		     or,
		     p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
		     c, c, c, c,
		     uvp, vec2f_add(uvp, vec2f(uvs.x, 0)), vec2f_add(uvp, vec2f(0, uvs.y)), vec2f_add(uvp, uvs));
}

static const char *shader_type_as_cstr(GLuint shader)
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

static bool compile_shader_file(const char *file_path, GLenum shader_type, GLuint *shader)
{
    char *content = slurp_file(file_path, NULL);
    if (content == 0) {
        fprintf(stderr, "ERROR: failed to load `%s` shader file: %s\n", file_path, strerror(errno));
        return false;
    }

    if (!compile_shader_source(content, shader_type, shader)) {
        fprintf(stderr, "ERROR: failed to compile `%s` shader file\n", file_path);
	free(content);
        return false;
    }

    free(content);
    return true;
}

static bool compile_shader_source(const GLchar *source, GLenum shader_type, GLuint *shader)
{
    *shader = glCreateShader(shader_type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);

    GLint compiled = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLchar message[1024];
        GLsizei message_size = 0;
        glGetShaderInfoLog(*shader, sizeof(message), &message_size, message);
        fprintf(stderr, "ERROR: could not compile %s\n", shader_type_as_cstr(shader_type));
        fprintf(stderr, "%.*s\n", message_size, message);
        return false;
    }

    return true;
}

static void attach_shaders_to_program(GLuint *shaders, size_t shaders_count, GLuint program)
{
    for (size_t i = 0; i < shaders_count; ++i) {
        glAttachShader(program, shaders[i]);
    }
}

static bool link_program(GLuint program, const char *file_path, size_t line)
{
    glLinkProgram(program);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLsizei message_size = 0;
        GLchar message[1024];

        glGetProgramInfoLog(program, sizeof(message), &message_size, message);
        fprintf(stderr, "%s:%zu: Program Linking: %.*s\n", file_path, line, message_size, message);
    }

    return linked;
}

size_t open_renderer_push_texture(Open_Renderer *or, Open_Texture texture) {
  not_null(or);

  glActiveTexture(open_renderer_index_to_texture(or->images_count));
  glGenTextures(1, &or->textures);
  glBindTexture(GL_TEXTURE_2D, or->textures);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if(texture.grey) {
    glTexImage2D(GL_TEXTURE_2D,
		 0,
		 GL_RED,
		 texture.width,
		 texture.height,
		 0,
		 GL_RED,
		 GL_UNSIGNED_BYTE,
		 texture.data);
  } else {
    glTexImage2D(GL_TEXTURE_2D,
		 0,
		 GL_RGBA,
		 texture.width,
		 texture.height,
		 0,
		 GL_RGBA,
		 GL_UNSIGNED_INT_8_8_8_8_REV,
		 texture.data);
  }


  return or->images_count++;
}

void open_renderer_flush_textures(Open_Renderer *or) {
  not_null(or);
  glDeleteTextures(or->images_count, &or->textures);
  or->images_count = 0;
}

#endif //OPEN_RENDERER_IMPLEMENTATION

#endif //GL_RENDERER_H_
