#ifndef GUI_H_H
#define GUI_H_H

#ifdef _WIN32
#include <windows.h>
#include <GL/GL.h>

#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5

#define GL_ARRAY_BUFFER 0x8892
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_ACTIVE_UNIFORMS 0x8B86

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367

typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;

PROC _glActiveTexture = NULL;
void glActiveTexture(GLenum texture) { _glActiveTexture(texture); }

PROC _glGenVertexArrays = NULL;
void glGenVertexArrays(GLsizei n, GLuint *arrays) { _glGenVertexArrays(n, arrays); }

PROC _glBindVertexArray = NULL;
void glBindVertexArray(GLuint array) { _glBindVertexArray(array); }

PROC _glGenBuffers = NULL;
void glGenBuffers(GLsizei n, GLuint *buffers) { _glGenBuffers(n, buffers); }

PROC _glBindBuffer = NULL;
void glBindBuffer(GLenum target, GLuint buffer) { _glBindBuffer(target, buffer); }

PROC _glBufferData = NULL;
void glBufferData(GLenum target, GLsizeiptr size, const void * data, GLenum usage) { _glBufferData(target, size, data, usage); }

PROC _glEnableVertexAttribArray = NULL;
void glEnableVertexAttribArray(GLuint index) { _glEnableVertexAttribArray(index); }

PROC _glVertexAttribPointer = NULL;
void glVertexAttribPointer(GLuint index,
			   GLint size,
			   GLenum type,
			   GLboolean normalized,
			   GLsizei stride,
			   const void * pointer) {
  _glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

PROC _glCreateShader = NULL;
GLuint glCreateShader(GLenum shaderType) { return _glCreateShader(shaderType); }

PROC _glShaderSource = NULL;
void glShaderSource(GLuint shader,
		    GLsizei count,
		    const GLchar **string,
		    const GLint *length) {
  _glShaderSource(shader, count, string, length);
}

PROC _glCompileShader = NULL;
void glCompileShader(GLuint shader) { _glCompileShader(shader); }

PROC _glGetShaderiv = NULL;
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
  _glGetShaderiv(shader, pname, params);
}

PROC _glGetShaderInfoLog = NULL;
void glGetShaderInfoLog(GLuint shader,
			GLsizei maxLength,
			GLsizei *length,
			GLchar *infoLog) {
  _glGetShaderInfoLog(shader, maxLength, length, infoLog);
}

PROC _glCreateProgram = NULL;
GLuint glCreateProgram(void) { return _glCreateProgram(); }

PROC _glAttachShader = NULL;
void glAttachShader(GLuint program, GLuint shader) { _glAttachShader(program, shader); }

PROC _glLinkProgram = NULL;
void glLinkProgram(GLuint program) { _glLinkProgram(program); }

PROC _glGetProgramInfoLog = NULL;
void glGetProgramInfoLog(GLuint program,
			 GLsizei maxLength,
			 GLsizei *length,
			 GLchar *infoLog) {
  _glGetProgramInfoLog(program, maxLength, length, infoLog);
}

PROC _glGetProgramiv = NULL;
void glGetProgramiv(GLuint program,
		    GLenum pname,
		    GLint *params) {
  _glGetProgramiv(program, pname, params);
}

PROC _glUseProgram = NULL;
void glUseProgram(GLuint program) { _glUseProgram(program); }

PROC _glBufferSubData = NULL;
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void * data) {
  _glBufferSubData(target, offset, size, data);
}

PROC _glUniform2f = NULL;
void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
  _glUniform2f(location, v0, v1);
}

PROC _glUniform1f = NULL;
void glUniform1f(GLint location, GLfloat v0) {
  _glUniform1f(location, v0);
}

PROC _glUniform1fv = NULL;
void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
  _glUniform1fv(location, count, value);
}

PROC _glUniform2fv = NULL;
void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
  _glUniform2fv(location, count, value);
}

PROC _glUniform1i = NULL;
void glUniform1i(GLint location, GLint v0) {
  _glUniform1i(location, v0);
}

PROC _glGetUniformLocation = NULL;
GLint glGetUniformLocation(GLuint program, const GLchar *name) { return _glGetUniformLocation(program, name); }

PROC _glGetActiveUniform = NULL;
void glGetActiveUniform(GLuint program,
			GLuint index,
			GLsizei bufSize,
			GLsizei *length,
			GLint *size,
			GLenum *type,
			GLchar *name) {
  _glGetActiveUniform(program, index, bufSize, length, size, type, name);
}

PROC _glGetUniformfv = NULL;
void glGetUniformfv(GLuint program, GLint location, GLfloat *params) {
  _glGetUniformfv(program, location, params);
}

PROC _glGetUniformiv = NULL;
void glGetUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint *params) {
  _glGetUniformiv(program, location, bufSize, params);
}

#endif //_WIN32

#include <stdbool.h>

#ifndef GUI_DEF
#define GUI_DEF static inline
#endif //GUI_DEF

typedef struct{
#ifdef _WIN32
  HWND win;
  HDC dc;
  RECT rect;
#endif //_WIN32
  bool running;
}Gui;

typedef struct{
#ifdef _WIN32
  MSG msg;
#endif //_WIN32
}Gui_Event;

#ifdef _WIN32
GUI_DEF bool gui_init(Gui *gui, HINSTANCE hInstance, int nCmdShow, const char *name);
GUI_DEF void win32_opengl_init();
#endif //_WIN32
GUI_DEF bool gui_get_window_sizef(Gui *gui, float *width, float *height);
GUI_DEF bool gui_peek(Gui *gui, Gui_Event *event);
GUI_DEF bool gui_free(Gui *gui);

GUI_DEF bool gui_init_opengl(Gui *gui); //-lgdi32 -lopengl32
GUI_DEF void gui_swap_buffers(Gui *gui);

#ifdef GUI_IMPLEMENTATION

LRESULT CALLBACK Gui_Implementation_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if(message == WM_DESTROY) {
    Gui *gui = (Gui *) GetWindowLongPtr(hWnd, 0);
    if(gui != NULL) {
      gui->running = false;
    }
    return 0;
  } else {
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
}

#ifdef _WIN32
GUI_DEF bool gui_init(Gui *gui, HINSTANCE hInstance, int nCmdShow, const char *name) {
  
  WNDCLASS wc = {0};
  wc.lpfnWndProc = Gui_Implementation_WndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = name;
  wc.cbWndExtra = sizeof(LONG_PTR);

  if(!RegisterClass(&wc)) {
    return false;
  }

  if(!(gui->win = CreateWindowEx(0,
				 wc.lpszClassName,
				 wc.lpszClassName,
				 WS_OVERLAPPEDWINDOW,
				 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				 NULL,
				 NULL,
				 hInstance,
				 NULL))) {
    return false;
  }

  gui->dc = GetDC(gui->win);

  LONG_PTR lptr = {0};
  memcpy(&lptr, &gui, sizeof(gui));  
  SetWindowLongPtr(gui->win, 0, lptr);

  ShowWindow(gui->win, nCmdShow);
  UpdateWindow(gui->win);

  return true;
}

GUI_DEF void win32_opengl_init() {
  if(_glActiveTexture != NULL) {
    return;
  }
  _glActiveTexture = wglGetProcAddress("glActiveTexture");
  _glGenVertexArrays = wglGetProcAddress("glGenVertexArrays");
  _glBindVertexArray= wglGetProcAddress("glBindVertexArray");
  _glGenBuffers= wglGetProcAddress("glGenBuffers");
  _glBindBuffer= wglGetProcAddress("glBindBuffer");
  _glBufferData= wglGetProcAddress("glBufferData");
  _glEnableVertexAttribArray= wglGetProcAddress("glEnableVertexAttribArray");
  _glVertexAttribPointer= wglGetProcAddress("glVertexAttribPointer");
  _glCreateShader= wglGetProcAddress("glCreateShader");
  _glShaderSource= wglGetProcAddress("glShaderSource");
  _glCompileShader= wglGetProcAddress("glCompileShader");
  _glGetShaderiv= wglGetProcAddress("glGetShaderiv");
  _glGetShaderInfoLog= wglGetProcAddress("glGetShaderInfoLog");
  _glCreateProgram= wglGetProcAddress("glCreateProgram");
  _glAttachShader= wglGetProcAddress("glAttachShader");
  _glLinkProgram= wglGetProcAddress("glLinkProgram");
  _glGetProgramInfoLog= wglGetProcAddress("glGetProgramInfoLog");
  _glGetProgramiv= wglGetProcAddress("glGetProgramiv");
  _glUseProgram= wglGetProcAddress("glUseProgram");
  _glBufferSubData= wglGetProcAddress("glBufferSubData");
  _glUniform2f= wglGetProcAddress("glUniform2f");
  _glUniform1f= wglGetProcAddress("glUniform1f");
  _glUniform1i= wglGetProcAddress("glUniform1i");
  _glGetUniformLocation= wglGetProcAddress("glGetUniformLocation");
  _glGetActiveUniform= wglGetProcAddress("glGetActiveUniform");
  _glGetUniformfv= wglGetProcAddress("glGetUniformfv");
  _glUniform1fv= wglGetProcAddress("glUniform1fv");
  _glUniform2fv= wglGetProcAddress("glUniform2fv");
  _glGetUniformiv= wglGetProcAddress("glGetUniformiv");
}

GUI_DEF bool gui_get_window_sizef(Gui *gui, float *width, float *height) {
  bool result = GetClientRect(gui->win, &gui->rect);
  if(result) {
    *width = (float) (gui->rect.right - gui->rect.left);
    *height = (float) (gui->rect.bottom - gui->rect.top);
  }
  return result;
}

GUI_DEF bool gui_peek(Gui *gui, Gui_Event *event) {
  MSG *msg = &event->msg;
  bool result = PeekMessage(msg, gui->win, 0, 0, PM_REMOVE);
  if(result) {
    TranslateMessage(msg);
    DispatchMessage(msg);
  }
  return result;
}

GUI_DEF bool gui_init_opengl(Gui *gui) {
  HDC windowDC = GetDC(gui->win);

  PIXELFORMATDESCRIPTOR desiredFormat = {0};
  desiredFormat.nSize = sizeof(desiredFormat);
  desiredFormat.nVersion = 1;
  desiredFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
  desiredFormat.cColorBits = 32;
  desiredFormat.cAlphaBits = 8;

  int suggestedFormatIndex = ChoosePixelFormat(windowDC, &desiredFormat);
  PIXELFORMATDESCRIPTOR suggestedFormat;
  DescribePixelFormat(windowDC, suggestedFormatIndex, sizeof(suggestedFormat), &suggestedFormat);
  SetPixelFormat(windowDC, suggestedFormatIndex, &suggestedFormat);
  
  HGLRC openglRC = wglCreateContext(windowDC);
  if(!wglMakeCurrent(windowDC, openglRC)) {
    return false;
  }
  ReleaseDC(gui->win, windowDC);
  
  win32_opengl_init();
  
  return true;
}

GUI_DEF bool gui_free(Gui *gui) {
  return DestroyWindow(gui->win);
}

GUI_DEF void gui_swap_buffers(Gui *gui) {
  SwapBuffers(gui->dc);
}

#endif //_WIN32

#endif //GUI_IMPLEMENTATION

#endif //GUI_H_H















