#ifndef GUI_H_H
#define GUI_H_H

#ifdef linux
#include <time.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>

GLuint (*glCreateShader)(GLenum shaderType) = NULL;
void (*glShaderSource) (GLuint shader,
			GLsizei count,
			const GLchar **string,
			const GLint *length)= NULL;
void (*glCompileShader)(GLuint shader) = NULL;
void (*glGetShaderiv)(GLuint shader, GLenum pname, GLint *params) = NULL;
void (*glGetShaderInfoLog)(GLuint shader,
			   GLsizei maxLength,
			   GLsizei *length,
			   GLchar *infoLog) = NULL;
GLuint (*glCreateProgram)(void) = NULL;
void (*glAttachShader)(GLuint program, GLuint shader) = NULL;
void (*glLinkProgram)(GLuint program) = NULL;
void (*glGetProgramiv)(GLuint program,
		       GLenum pname,
		       GLint *params) = NULL;
void (*glGetProgramInfoLog)(GLuint program,
			    GLsizei maxLength,
			    GLsizei *length,
			    GLchar *infoLog) = NULL;
void (*glGenVertexArrays)(GLsizei n, GLuint *arrays) = NULL;
void (*glBindVertexArray)(GLuint array) = NULL;
void (*glGenBuffers)(GLsizei n, GLuint *buffers) = NULL;
void (*glBindBuffer)(GLenum target, GLuint buffer) = NULL;
void (*glBufferData)(GLenum target, GLsizeiptr size, const void * data, GLenum usage) = NULL;
void (*glEnableVertexAttribArray)(GLuint index) = NULL;
void (*glVertexAttribPointer)(GLuint index,
			      GLint size,
			      GLenum type,
			      GLboolean normalized,
			      GLsizei stride,
			      const void * pointer) = NULL;
void (*glUseProgram)(GLuint program) = NULL;

void (*glUniform2f)(GLint location, GLfloat v0, GLfloat v1) = NULL;
void (*glUniform1f)(GLint location, GLfloat v0) = NULL;
void (*glUniform1fv)(GLint location, GLsizei count, const GLfloat *value) = NULL;
void (*glUniform2fv)(GLint location, GLsizei count, const GLfloat *value) = NULL;
GLint (*glGetUniformLocation)(GLuint program, const GLchar *name) = NULL;
void (*glUniform1i)(GLint location, GLint v0) = NULL;
void (*glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void * data) = NULL;

#endif //linux

#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#include <GL/GL.h> //link with -opengl32 -lgdi32
                   //or        opengl32.lib gdi32.lib user32.lib
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

#define GL_BGRA 0x80E1

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

PROC _wglSwapIntervalEXT = NULL;
int wglSwapIntervalEXT(GLint interval) {
    return _wglSwapIntervalEXT(interval);
}

#endif //_WIN32

#include <stdbool.h>

#ifndef GUI_DEF
#define GUI_DEF static inline
#endif //GUI_DEF

typedef enum{
  GUI_EVENT_NONE = 0,
  GUI_EVENT_KEYRELEASE,
  GUI_EVENT_KEYPRESS,
  GUI_EVENT_MOUSEPRESS,
  GUI_EVENT_MOUSERELEASE,
  GUI_EVENT_MOUSEMOTION,
  GUI_EVENT_COUNT,
}Gui_Event_Type;

typedef struct{
#ifdef linux
  XEvent x_event;
#endif //linux
  
#ifdef _WIN32
  MSG msg;
  POINT cursor;
#endif //_WIN32
  Gui_Event_Type type;  
  char key;
  int mousex;
  int mousey;
}Gui_Event;

typedef struct{
  unsigned int width, height;
  void *data;
}Gui_Canvas;

typedef struct{
#ifdef linux
  struct timespec counter;
#endif //linux
  
#ifdef _WIN32
  LARGE_INTEGER counter;
#endif //_WIN32<
}Gui_Time;

typedef struct{
#ifdef linux
  Display *display;
  GLXContext ctx;
  Window win;
  Colormap cmap;
  int fd;
  fd_set in_fds;
  struct timeval tv;
  int wmDeleteMessage;
#endif //linux
  
#ifdef _WIN32  
  HWND win;
  HDC dc;
  RECT rect;
  BITMAPINFO info;
#endif //_WIN32
  bool running;
  Gui_Canvas *canvas;
}Gui;

#ifdef _WIN32
GUI_DEF bool gui_init(Gui *gui, Gui_Canvas *canvas,  char *name);
GUI_DEF void win32_opengl_init();
#endif //_WIN32
GUI_DEF void gui_render_canvas(Gui *gui);
GUI_DEF void gui_toggle_fullscreen(Gui *gui);
GUI_DEF bool gui_get_window_size(Gui *gui, int *width, int *height);
GUI_DEF bool gui_get_window_sizef(Gui *gui, float *width, float *height);
GUI_DEF void gui_time_capture(Gui_Time *time);
GUI_DEF unsigned long gui_time_measure(Gui_Time *reference);
GUI_DEF bool gui_peek(Gui *gui, Gui_Event *event);
GUI_DEF bool gui_free(Gui *gui);

GUI_DEF void gui_mouse_to_screen(int width, int height, int *mousex, int *mousey);
GUI_DEF void gui_mouse_to_screenf(float width, float height, float *mousex, float *mousey);
  
GUI_DEF bool gui_init_opengl(Gui *gui); //-lgdi32 -lopengl32
GUI_DEF bool gui_use_vsync(int use);
GUI_DEF void gui_swap_buffers(Gui *gui);

#ifdef GUI_IMPLEMENTATION

#ifdef _MSC_VER
#  pragma comment(lib,"gdi32.lib")
#  pragma comment(lib,"user32.lib")
#  ifdef GUI_OPENGL
#    pragma comment(lib,"opengl32.lib")
#  endif //GUI_OPENGL
#endif //_WIN32

#ifdef linux
GUI_DEF bool gui_init(Gui *gui, Gui_Canvas *canvas, const char *name) {

  *gui = (Gui) {0};

  gui->display = XOpenDisplay(NULL);
  if(!gui->display) {
    return false;
  }

  static int visual_attribs[] =
    {
     GLX_X_RENDERABLE    , True,
     GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
     GLX_RENDER_TYPE     , GLX_RGBA_BIT,
     GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
     GLX_RED_SIZE        , 8,
     GLX_GREEN_SIZE      , 8,
     GLX_BLUE_SIZE       , 8,
     GLX_ALPHA_SIZE      , 8,
     GLX_DEPTH_SIZE      , 24,
     GLX_STENCIL_SIZE    , 8,
     GLX_DOUBLEBUFFER    , True,
     //GLX_SAMPLE_BUFFERS  , 1,
     //GLX_SAMPLES         , 4,
     None
    };

  int fbcount;
  GLXFBConfig* fbc = glXChooseFBConfig(gui->display, DefaultScreen(gui->display), visual_attribs, &fbcount);
  if(!fbc) {
    return false;
  }
  
  int best_fbc = -1;
  int best_num_samples = -1;
  for(int i=0;i<fbcount;i++) {
    XVisualInfo *vi = glXGetVisualFromFBConfig(gui->display, fbc[i]);
    if(vi) {
      int samp_buf, samples;
      glXGetFBConfigAttrib(gui->display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
      glXGetFBConfigAttrib(gui->display, fbc[i], GLX_SAMPLES       , &samples  );

      if(best_fbc < 0 || (samp_buf && samples > best_num_samples)) {
	best_fbc = i;
	best_num_samples = samples;
      }
    }
    XFree(vi);
  }

  GLXFBConfig bestFbc = fbc[best_fbc];
  XVisualInfo *vi = glXGetVisualFromFBConfig(gui->display, bestFbc);
  XFree(fbc);

  XSetWindowAttributes swa;
  swa.colormap = gui->cmap = XCreateColormap(gui->display,
					     RootWindow(gui->display, vi->screen), 
					     vi->visual, AllocNone);
  swa.background_pixmap = None ;
  swa.border_pixel      = 0;
  swa.event_mask        = StructureNotifyMask;

  int width = 800;
  int height = 600;
  if(canvas != NULL) {
    width = canvas->width;
    height = canvas->height;
  }

  if(!gui_init_opengl(gui)) {
    return false;
  }
  
  gui->win = XCreateWindow(gui->display, RootWindow(gui->display, vi->screen), 
			   0, 0, width, height, 0, vi->depth, InputOutput, 
			   vi->visual, 
			   CWBorderPixel|CWColormap|CWEventMask, &swa);
  XFree(vi);
  if(!gui->win) {
    return false;
  }

  Atom wmDeleteMessage = XInternAtom(gui->display, "WM_DELETE_WINDOW", False);
  gui->wmDeleteMessage = (int) wmDeleteMessage;
  XSetWMProtocols(gui->display, gui->win, &wmDeleteMessage, 1);
  XStoreName(gui->display, gui->win, name);
  XSelectInput(gui->display, gui->win,
	       ButtonPressMask | ButtonReleaseMask |
	       ExposureMask |
	       PointerMotionMask |
	       KeyPressMask | StructureNotifyMask);
  XMapWindow(gui->display, gui->win);

  gui->ctx = glXCreateNewContext(gui->display, bestFbc, GLX_RGBA_TYPE, 0, True );
  XSync(gui->display, False);  
  glXMakeCurrent(gui->display, gui->win, gui->ctx);

  gui->running = true;

  /////////////////////////////////////////

  gui->fd = -1; //ConnectionNumber(gui->display);  
  gui->tv.tv_usec = 0;
  gui->tv.tv_sec = 1;  
  return true;
}

GUI_DEF bool gui_peek(Gui *gui, Gui_Event *event) {

  event->type = GUI_EVENT_NONE;

  if(gui->fd < 0) {
    gui->fd = ConnectionNumber(gui->display);
    
    return false;
  }
  XEvent *e = &event->x_event;

  /*
  FD_SET(gui->fd, &gui->in_fds);
  FD_ZERO(&gui->in_fds);
  gui->tv.tv_usec = 0;
  gui->tv.tv_sec = 1;

  int hasEvent = 1;
  int num_ready_fds = select(gui->fd + 1, &gui->in_fds, NULL, NULL, &gui->tv);
  if(num_ready_fds == 0) {
    hasEvent = 0;
  }
  
  */
  if(XPending(gui->display)) {
    XNextEvent(gui->display, e);
    if(e->type == ClientMessage) {
      if(e->xclient.data.l[0] == gui->wmDeleteMessage) {
	gui->running = 0;
      }
    } else if(e->type == ButtonPress ||
	      e->type == ButtonRelease) {
      event->type = e->type == ButtonPress ?
	GUI_EVENT_MOUSEPRESS : GUI_EVENT_MOUSERELEASE;
      if(e->xbutton.button == Button1) {
	event->key = 'L';
      } else if(e->xbutton.button == Button2) {
	event->key = 'M';
      } else if(e->xbutton.button == Button3) {
	event->key = 'R';
      } 
    } else if(e->type == MotionNotify) {
      event->type = GUI_EVENT_MOUSEMOTION;
      event->mousex = e->xmotion.x;
      event->mousey = e->xmotion.y;
    } else if(e->type == KeyPress ||
	      e->type == KeyRelease) {
      event->type = GUI_EVENT_KEYPRESS;
      if(e->type == KeyRelease) {
	event->type = GUI_EVENT_KEYRELEASE;
      }
      event->key = XkbKeycodeToKeysym(gui->display, e->xkey.keycode, 0, 1);
    } 
    return true;
  }

  return false;
}

GUI_DEF bool gui_get_window_size(Gui *gui, int *width, int *height) {

  Window root;
  int x, y;
  unsigned int w, h, bw, depth;
  XGetGeometry(gui->display, gui->win, &root, &x, &y, (unsigned int *)&w, (unsigned int *)&h, &bw, &depth);
  
  *width = (int) w;
  *height = (int) h;  
  return false;  
}

GUI_DEF bool gui_get_window_sizef(Gui *gui, float *width, float *height) {
  Window root;
  int x, y;
  unsigned int w, h, bw, depth;
  XGetGeometry(gui->display, gui->win, &root, &x, &y, &w, &h, &bw, &depth);
  
  *width = (float) w;
  *height = (float) h;
  return true;
}

//TODO
GUI_DEF void gui_render_canvas(Gui *gui) {
  (void) gui;
}

//TODO
GUI_DEF void gui_toggle_fullscreen(Gui *gui) {
  (void) gui;
}

//TODO
GUI_DEF bool gui_use_vsync(int use) {
  (void) use;
  return true;
}

GUI_DEF void gui_time_capture(Gui_Time *time) {
  clock_gettime(CLOCK_MONOTONIC, &time->counter);
}

GUI_DEF unsigned long gui_time_measure(Gui_Time *reference) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  
  unsigned long ms = (now.tv_sec - reference->counter.tv_sec) * 1000 +
    (now.tv_nsec - reference->counter.tv_nsec) / 1000 / 1000;

  reference->counter = now;
  return ms;
}

GUI_DEF void gui_swap_buffers(Gui *gui) {
  glXSwapBuffers(gui->display, gui->win);
}

GUI_DEF bool gui_free(Gui *gui) {
  
  glXMakeCurrent(gui->display, 0, 0 );
  glXDestroyContext(gui->display, gui->ctx);
  XDestroyWindow(gui->display, gui->win);
  XFreeColormap(gui->display, gui->cmap);
  XCloseDisplay(gui->display);

  return true;
}

GUI_DEF bool gui_init_opengl(Gui *gui) {
  (void) gui;
  if(glCreateShader != NULL) {
    return true;
  }
  
  glCreateShader = (GLuint (*)(GLenum)) glXGetProcAddressARB((unsigned char *) "glCreateShader");

  glShaderSource = (void (*)(GLuint, GLsizei, const GLchar **, const GLint *)) glXGetProcAddressARB((unsigned char *) "glShaderSource");
  
  glCompileShader = (void (*)(GLuint)) glXGetProcAddressARB((unsigned char *) "glCompileShader");
  
  glGetShaderiv = (void (*)(GLuint, GLenum, GLint *)) glXGetProcAddressARB((unsigned char *) "glGetShaderiv");
  
  glGetShaderInfoLog = (void (*)(GLuint, GLsizei , GLsizei *, GLchar *)) glXGetProcAddressARB((unsigned char *) "glGetShaderInfoLog");

  glCreateProgram = (GLuint (*)(void)) glXGetProcAddressARB((unsigned char *) "glCreateProgram");
  
  glAttachShader = (void (*)(GLuint, GLuint)) glXGetProcAddressARB((unsigned char *) "glAttachShader");
    
  glLinkProgram = (void (*)(GLuint)) glXGetProcAddressARB((unsigned char *) "glLinkProgram");
  
  glGetProgramiv = (void (*)(GLuint, GLenum, GLint *)) glXGetProcAddressARB((unsigned char *) "glGetProgramiv");

  glGetProgramInfoLog = (void (*)(GLuint, GLsizei, GLsizei *, GLchar *)) glXGetProcAddressARB((unsigned char *) "glGetProgramInfoLog");

  glGenVertexArrays = (void (*)(GLsizei, GLuint *)) glXGetProcAddressARB((unsigned char *) "glGenVertexArrays");

  glBindVertexArray = (void (*)(GLuint)) glXGetProcAddressARB((unsigned char *) "glBindVertexArray");

  glGenBuffers = (void (*)(GLsizei, GLuint *)) glXGetProcAddressARB((unsigned char *) "glGenBuffers");

  glBindBuffer = (void (*)(GLenum, GLuint)) glXGetProcAddressARB((unsigned char *) "glBindBuffer");

  glBufferData = (void (*)(GLenum, GLsizeiptr, const void *, GLenum)) glXGetProcAddressARB((unsigned char *) "glBufferData");

  glEnableVertexAttribArray = (void (*)(GLuint)) glXGetProcAddressARB((unsigned char *) "glEnableVertexAttribArray");
  
  glVertexAttribPointer = (void (*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *)) glXGetProcAddressARB((unsigned char *) "glVertexAttribPointer");

  glUseProgram = (void (*)(GLuint)) glXGetProcAddressARB((unsigned char *) "glUseProgram");

  glUseProgram = (void (*)(GLuint)) glXGetProcAddressARB((unsigned char *) "glUseProgram");
  
  glUniform2f = (void (*)(GLint, GLfloat, GLfloat)) glXGetProcAddressARB((unsigned char *) "glUniform2f");
  glUniform1f = (void (*)(GLint, GLfloat)) glXGetProcAddressARB((unsigned char *) "glUniform1f");
  
  glUniform1fv = (void (*)(GLint, GLsizei, const GLfloat *)) glXGetProcAddressARB((unsigned char *) "glUniform1fv");

  glUniform2fv = (void (*)(GLint, GLsizei, const GLfloat *)) glXGetProcAddressARB((unsigned char *) "glUniform2fv");

  glGetUniformLocation = (GLint (*)(GLuint, const GLchar *)) glXGetProcAddressARB((unsigned char *) "glGetUniformLocation");
  
  glUniform1i = (void (*)(GLint, GLint)) glXGetProcAddressARB((unsigned char *) "glUniform1i");

  glBufferSubData = (void (*)(GLenum, GLintptr, GLsizeiptr, const void *)) glXGetProcAddressARB((unsigned char *) "glBufferSubData");  

  return true;
}

#endif //linux

GUI_DEF void gui_mouse_to_screen(int width, int height, int *mousex, int *mousey) {
  *mousey = height - *mousey;
  if(*mousey < 0) *mousey = 0;
  if(*mousey >= height) *mousey = height - 1;
  if(*mousex < 0) *mousex = 0;
  if(*mousex >= width) *mousex = width - 1;
}

GUI_DEF void gui_mouse_to_screenf(float width, float height, float *mousex, float *mousey) {
  *mousey = height - *mousey;
  if(*mousey < 0) *mousey = 0;
  if(*mousey >= height) *mousey = height - 1;
  if(*mousex < 0) *mousex = 0;
  if(*mousex >= width) *mousex = width - 1;
}

#ifdef _WIN32
static LARGE_INTEGER guiWin32PerfCountFrequency;
static HCURSOR guiDefaultCursor;

LRESULT CALLBACK Gui_Implementation_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if(message == WM_CLOSE) {
    Gui *gui = (Gui *) GetWindowLongPtr(hWnd, 0);
    if(gui != NULL) {
      gui->running = false;
    }
    PostQuitMessage(0);
    return 0;
  } else if(message == WM_DESTROY) {
    Gui *gui = (Gui *) GetWindowLongPtr(hWnd, 0);
    if(gui != NULL) {
      gui->running = false;
    }
    return 0;
  } else if(message == WM_PAINT){
    Gui *gui = (Gui *) GetWindowLongPtr(hWnd, 0);
    if(gui != NULL && gui->canvas != NULL) {
      PAINTSTRUCT ps;
      HDC context = BeginPaint(gui->win, &ps);
      StretchDIBits(context,
		    0, 0, gui->canvas->width, gui->canvas->height,
		    0, 0, gui->canvas->width, gui->canvas->height,
		    gui->canvas->data,
		    &gui->info,
		    DIB_RGB_COLORS, SRCCOPY);
      EndPaint(gui->win, &ps);
      return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
  } else if(message == WM_SETCURSOR) {
    SetCursor(guiDefaultCursor);
    return DefWindowProc(hWnd, message, wParam, lParam);
  } else {
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
}

GUI_DEF bool gui_init(Gui *gui, Gui_Canvas *canvas,  char *name) {

#ifndef GUI_CONSOLE
  FreeConsole();
#endif //GUI_CONSOLE
  
  if(!guiWin32PerfCountFrequency.QuadPart) {
    QueryPerformanceFrequency(&guiWin32PerfCountFrequency);
  }
  
  guiDefaultCursor = LoadCursor(0, IDC_ARROW);

  HMODULE hInstance = GetModuleHandle(NULL);

  STARTUPINFO startupInfo;
  GetStartupInfo(&startupInfo);
  DWORD nCmdShow = startupInfo.wShowWindow;
  
  WNDCLASS wc = {0};
  wc.lpfnWndProc = Gui_Implementation_WndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = name;
  //wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wc.cbWndExtra = sizeof(LONG_PTR);

  if(!RegisterClass(&wc)) {
    return false;
  }

  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  if(!(gui->win = CreateWindowEx(0,
				 wc.lpszClassName,
				 wc.lpszClassName,
				 WS_OVERLAPPEDWINDOW,
				 canvas != NULL
				 ? (screenWidth / 2 - canvas->width/2)
				 : CW_USEDEFAULT,
				 canvas != NULL
				 ? (screenHeight / 2 - (canvas->height + 39)/2)
				 : CW_USEDEFAULT,
				 canvas != NULL ? canvas->width : CW_USEDEFAULT,
				 canvas != NULL ? canvas->height + 39 : CW_USEDEFAULT,
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

  if(canvas != NULL && canvas->data != NULL)  {
    gui->info = (BITMAPINFO) {0};
    gui->info.bmiHeader.biSize = sizeof(gui->info.bmiHeader);
    gui->info.bmiHeader.biWidth = canvas->width;
#ifdef GUI_UPSIDEDOWN
    gui->info.bmiHeader.biHeight = -canvas->height;
#else
    gui->info.bmiHeader.biHeight = canvas->height;
#endif //GUI_UPSIDEDOWN
    gui->info.bmiHeader.biBitCount = 32;
    gui->info.bmiHeader.biPlanes = 1;    
    gui->info.bmiHeader.biCompression = BI_RGB;
  } else {
    if(!gui_init_opengl(gui)) {
      return false;
    }
  }

  if(canvas != NULL) {
    gui->canvas = canvas;
  } else {
    gui->canvas = (Gui_Canvas *)(void *) gui;
  }

  gui->running = true;

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
  _wglSwapIntervalEXT = wglGetProcAddress("wglSwapIntervalEXT");
}

GUI_DEF void gui_render_canvas(Gui *gui) {
  StretchDIBits(gui->dc,
		0, 0, gui->canvas->width, gui->canvas->height,
		0, 0, gui->canvas->width, gui->canvas->height,
		gui->canvas->data,
		&gui->info,
		DIB_RGB_COLORS, SRCCOPY);
}

GUI_DEF void gui_toggle_fullscreen(Gui *gui) {
    static WINDOWPLACEMENT windowPlacement = {0};
    windowPlacement.length = sizeof(windowPlacement);
    DWORD style = GetWindowLong(gui->win, GWL_STYLE);

  if(style & WS_OVERLAPPEDWINDOW) {
      MONITORINFO monitorInfo = {0};
      monitorInfo.cbSize = sizeof(monitorInfo);
    if(GetWindowPlacement(gui->win, &windowPlacement) &&
       GetMonitorInfo(MonitorFromWindow(gui->win, MONITOR_DEFAULTTOPRIMARY), &monitorInfo)) {
      SetWindowLong(gui->win, GWL_STYLE, style & ~WS_OVERLAPPED);
      SetWindowPos(gui->win, HWND_TOP,
		   monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
		   monitorInfo.rcMonitor.right -  monitorInfo.rcMonitor.left,
		   monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
		   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
  } else {
    SetWindowLong(gui->win, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(gui->win, &windowPlacement);
    SetWindowPos(gui->win, 0, 0, 0, 0, 0,
		 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
		 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  }
}

GUI_DEF bool gui_get_window_size(Gui *gui, int *width, int *height) {
  bool result = GetClientRect(gui->win, &gui->rect);
  if(result) {
    *width = (gui->rect.right - gui->rect.left);
    *height = (gui->rect.bottom - gui->rect.top);
  }
  return result;
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
  event->type = 0;
  
  int old_mousex = event->mousex;
  int old_mousey = event->mousey;
  if(GetCursorPos(&event->cursor) && ScreenToClient(gui->win, &event->cursor)) {
    event->mousex = event->cursor.x;
    event->mousey = event->cursor.y;
    
    if(old_mousex != event->mousex ||
       old_mousey != event->mousey) {
      event->type = GUI_EVENT_MOUSEMOTION;
    }
  }

  if(event->type) {
    return true;
  }
  
  bool result = PeekMessage(msg, gui->win, 0, 0, PM_REMOVE);
  if(!result) {
    return result;
  }

  TranslateMessage(msg);
  DispatchMessage(msg);

  switch(msg->message) {
  case WM_RBUTTONUP: 
  case WM_RBUTTONDOWN: {
      event->type = GUI_EVENT_MOUSEPRESS;
      if(msg->message == WM_RBUTTONUP) {
	  event->type = GUI_EVENT_MOUSERELEASE;	  
      }
      event->key = 'R';
  } break;
  case WM_LBUTTONUP: 
  case WM_LBUTTONDOWN: {
      event->type = GUI_EVENT_MOUSEPRESS;
      if(msg->message == WM_LBUTTONUP) {
	  event->type = GUI_EVENT_MOUSERELEASE;	  
      }
      event->key = 'L';
  } break;
  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP:
  case WM_KEYDOWN:
  case WM_KEYUP: {
    bool wasDown = ((msg->lParam & (1 << 30)) != 0);
    bool isDown = ((msg->lParam & (1 << 31)) == 0);

    if(wasDown != isDown) {
      event->key = msg->wParam;
      if(wasDown) {
	event->type = GUI_EVENT_KEYRELEASE;
      } else {
	event->type = GUI_EVENT_KEYPRESS;
      }
    }
  } break;
  default: {     
  } break;
  }
  
  return result;
}

GUI_DEF void gui_time_capture(Gui_Time *time) {
  QueryPerformanceCounter(&time->counter);
}

GUI_DEF unsigned long gui_time_measure(Gui_Time *reference) {
  LARGE_INTEGER endCounter;
  QueryPerformanceCounter(&endCounter);

  long elapsed = endCounter.QuadPart - reference->counter.QuadPart;
  reference->counter = endCounter;
  return (unsigned long) ((1000 * elapsed) / guiWin32PerfCountFrequency.QuadPart);
}

GUI_DEF bool gui_init_opengl(Gui *gui) {
#ifdef GUI_OPENGL
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
#else
  (void) gui;
#endif //GUI_OPENGL
  return true;
}

GUI_DEF bool gui_use_vsync(int use) {
    return wglSwapIntervalEXT(use);
}

GUI_DEF bool gui_free(Gui *gui) {
  ReleaseDC(gui->win, gui->dc);
  return DestroyWindow(gui->win);
}

GUI_DEF void gui_swap_buffers(Gui *gui) {
  SwapBuffers(gui->dc);
}

#endif //_WIN32

#endif //GUI_IMPLEMENTATION

#endif //GUI_H_H

