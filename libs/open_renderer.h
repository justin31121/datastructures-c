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

typedef enum {
	      OPEN_VERTEX_ATTR_POSITION = 0,
	      OPEN_VERTEX_ATTR_COLOR,
	      OPEN_VERTEX_ATTR_UV,
	      COUNT_OPEN_VERTEX_ATTRS
}Open_Vertex_Attribute;

typedef enum{
	     SHADER_FOR_COLOR = 0,
	     SHADER_FOR_IMAGE,
	     COUNT_SHADERS
}Open_Shader;

typedef struct{
  GLuint vao;
  GLuint vbo;
  GLuint programs[COUNT_SHADERS];

  Open_Vertex verticies[OPEN_RENDERER_VERTICIES_CAP];
  size_t verticies_count;

  Vec2i resolution;
}Open_Renderer;

bool open_renderer_init(Open_Renderer *or, const char *vert_file_path, const char *color_frag_file_path, const char *image_frag_file_path);
void open_renderer_set_shader(Open_Renderer *or, Open_Shader shader);
void open_renderer_flush(Open_Renderer *or);
void open_renderer_triangle(Open_Renderer *or, Vec2f p1, Vec2f p2, Vec2f p3, Vec4f c1, Vec4f c2, Vec4f c3, Vec2f uv1, Vec2f uv2, Vec2f uv3);
void open_renderer_quad(Open_Renderer *or, Vec2f p1, Vec2f p2, Vec2f p3, Vec2f p4, Vec4f c1, Vec4f c2, Vec4f c3, Vec4f c4, Vec2f uv1, Vec2f uv2, Vec2f uv3, Vec2f uv4);
void open_renderer_solid_rect(Open_Renderer *or, Vec2f pos, Vec2f size, Vec4f color);
void open_renderer_image_rect(Open_Renderer *or, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs);

#ifdef OPEN_RENDERER_IMPLEMENTATION

static bool compile_shader_file(const char *file_path, GLenum shader_type, GLuint *shader);
static void attach_shaders_to_program(GLuint *shaders, size_t shaders_count, GLuint program);
static bool link_program(GLuint program, const char *file_path, size_t line);
static bool compile_shader_source(const GLchar *source, GLenum shader_type, GLuint *shader);

bool open_renderer_init(Open_Renderer *or, const char *vert_file_path, const char *color_frag_file_path, const char *image_frag_file_path) {
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
  
  if(!compile_shader_file(vert_file_path, GL_VERTEX_SHADER, &shaders[0])) {
    return false;
  }

  //SHADER FOR COLOR
  if (!compile_shader_file(color_frag_file_path, GL_FRAGMENT_SHADER, &shaders[1])) {
    return false;
  }
  or->programs[SHADER_FOR_COLOR] = glCreateProgram();
  attach_shaders_to_program(shaders, sizeof(shaders) / sizeof(shaders[0]), or->programs[SHADER_FOR_COLOR]);
  if (!link_program(or->programs[SHADER_FOR_COLOR], __FILE__, __LINE__)) {
    return false;
  }

  //SHADER FOR IMAGE
  if (!compile_shader_file(image_frag_file_path, GL_FRAGMENT_SHADER, &shaders[1])) {
    return false;
  }
  or->programs[SHADER_FOR_IMAGE] = glCreateProgram();
  attach_shaders_to_program(shaders, sizeof(shaders) / sizeof(shaders[0]), or->programs[SHADER_FOR_IMAGE]);
  if (!link_program(or->programs[SHADER_FOR_IMAGE], __FILE__, __LINE__)) {
    return false;
  }

  return true;
}

void open_renderer_set_shader(Open_Renderer *or, Open_Shader shader) {
  not_null(or);

  glViewport(0, 0, or->resolution.x, or->resolution.y);
  glUseProgram(or->programs[shader]);

  glUniform2f(glGetUniformLocation(or->programs[shader], "resolution"), (float) or->resolution.x, (float) or->resolution.y);
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

#endif //OPEN_RENDERER_IMPLEMENTATION

#endif //GL_RENDERER_H_
