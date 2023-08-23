// Taken from: https://github.com/justin31121/datastructures-c

#ifndef GUI_H_H
#define GUI_H_H

#include <stdbool.h>
#include <string.h>

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
			const GLchar **_string,
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

#ifdef GUI_DRAG_N_DROP
#define XDND_PROTOCOL_VERSION 5

// State machine structure
typedef struct {
  bool xdndExchangeStarted;
  bool xdndPositionReceived;
  bool xdndStatusReceived;
  bool xdndStatusSent;
  bool xdndDropReceived;
  Time xdndDropTimestamp;
  Time xdndLastPositionTimestamp;
  bool amISource;
  int p_rootX;
  int p_rootY;
  Window otherWindow;
  Atom proposedAction;
  Atom proposedType;
} XDNDStateMachine;

// Atom definitions
static Atom XdndAware, XA_ATOM, XdndEnter, XdndPosition, XdndActionCopy, XdndLeave, XdndStatus, XdndDrop,
  XdndSelection, XDND_DATA, XdndTypeList, XdndFinished, WM_PROTOCOLS, WM_DELETE_WINDOW, typesWeAccept[6];

// XDND global state machine
static XDNDStateMachine xdndState;

#endif //GUI_DRAG_N_DROP

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
#define GL_RGB_INTEGER 0x8D98
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367

#define GL_BGRA 0x80E1
#define GL_RGB 0x1907
#define GL_BGR 0x80E0

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
GLuint glCreateShader(GLenum shaderType) { return (GLuint) _glCreateShader(shaderType); }

PROC _glShaderSource = NULL;
void glShaderSource(GLuint shader,
		    GLsizei count,
		    const GLchar **_string,
		    const GLint *length) {
  _glShaderSource(shader, count, _string, length);
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
GLuint glCreateProgram(void) { return (GLuint) _glCreateProgram(); }

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
GLint glGetUniformLocation(GLuint program, const GLchar *name) { return (GLint) _glGetUniformLocation(program, name); }

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
  return (int) _wglSwapIntervalEXT(interval);
}

#endif //_WIN32

#ifndef GUI_DEF
#define GUI_DEF static inline
#endif //GUI_DEF

typedef enum{
	     GUI_EVENT_NONE = 0,
	     GUI_EVENT_KEYRELEASE,
	     GUI_EVENT_KEYPRESS,
	     GUI_EVENT_MOUSEPRESS,
	     GUI_EVENT_MOUSERELEASE,
	     GUI_EVENT_MOUSEWHEEL,
	     GUI_EVENT_MOUSEMOTION,
	     GUI_EVENT_FILEDROP,
	     GUI_EVENT_DATARECEIVE,
	     GUI_EVENT_COUNT,
}Gui_Event_Type;

typedef struct{
  unsigned char *data;
  unsigned long size;
}Gui_Data;

typedef struct{
#ifdef linux
  XEvent x_event;
#endif //linux  
#ifdef _WIN32
  MSG msg;
  POINT cursor;
#endif //_WIN32
  Gui_Event_Type type;
  union{
    char key;
    int amount;
    long long value;
    Gui_Data data;
  }as;
  int mousex;
  int mousey;
}Gui_Event;

typedef struct{
#ifdef _WIN32
  HDROP h_drop;
  char path[MAX_PATH];
#else
  unsigned long numOfItems;
  unsigned long bytesAfterReturn;
  unsigned char *data;
#include <linux/limits.h>
  char path[PATH_MAX];
#endif //_WIN32
  int count;
  int index;
}Gui_Dragged_Files;

typedef struct{
#ifdef _WIN32
  HANDLE handle;
#else
  int s;
#endif //_WIN32  
}Gui_Clipboard;

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
  int clipboardAtom;

#ifdef GUI_DRAG_N_DROP
  bool clean_up_for_xdnd;
#endif //GUI_DRAG_N_DROP
  
#endif //linux
  
#ifdef _WIN32  
  HWND win;
  HDC dc;
  RECT rect;
  BITMAPINFO info;

  bool has_data;
  Gui_Data data;
#endif //_WIN32
  bool running;
  Gui_Canvas *canvas;
}Gui;

#ifdef _WIN32
GUI_DEF void win32_opengl_init();
#endif //_WIN32

GUI_DEF bool gui_init(Gui *gui, Gui_Canvas *canvas, const char *name);
GUI_DEF void gui_render_canvas(Gui *gui);
GUI_DEF void gui_toggle_fullscreen(Gui *gui);
GUI_DEF bool gui_get_window_size(Gui *gui, int *width, int *height);
GUI_DEF bool gui_get_window_sizef(Gui *gui, float *width, float *height);
GUI_DEF void gui_time_capture(Gui_Time *time);
GUI_DEF unsigned long gui_time_measure(Gui_Time *reference);GUI_DEF unsigned long gui_time_measure(Gui_Time *reference);
GUI_DEF bool gui_peek(Gui *gui, Gui_Event *event);
GUI_DEF bool gui_free(Gui *gui);

GUI_DEF bool gui_dragged_files_init(Gui_Dragged_Files *files, Gui_Event *event);
GUI_DEF bool gui_dragged_files_next(Gui_Dragged_Files *files, char **path);
GUI_DEF void gui_dragged_files_free(Gui_Dragged_Files *files);
GUI_DEF bool gui_open_file_dialog(char *buffer, size_t buffer_cap);

GUI_DEF bool gui_clipboard_init(Gui_Clipboard *clipboard, Gui *gui, char **text);
GUI_DEF void gui_clipboard_free(Gui_Clipboard *clipboard);

GUI_DEF void gui_mouse_to_screen(int width, int height, int *mousex, int *mousey);
GUI_DEF void gui_mouse_to_screenf(float width, float height, float *mousex, float *mousey);
  
GUI_DEF bool gui_init_opengl(Gui *gui); //-lgdi32 -lopengl32
GUI_DEF bool gui_use_vsync(int use);
GUI_DEF void gui_swap_buffers(Gui *gui);
GUI_DEF const char *gui_shader_type_as_cstr(GLuint shader);
GUI_DEF bool gui_compile_shader(GLuint *shader, GLenum shaderType, const char *shader_source);
GUI_DEF bool gui_link_program(GLuint *program, GLuint vertex_shader, GLuint fragment_shader);

#ifdef GUI_IMPLEMENTATION

#ifdef _MSC_VER
#  pragma comment(lib,"gdi32.lib")
#  pragma comment(lib,"user32.lib")
#  ifndef GUI_NO_OPENGL
#    pragma comment(lib,"opengl32.lib")
#  endif //GUI_NO_OPENGL
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
  gl_use_vsync(1);
  
  gui->win = XCreateWindow(gui->display, RootWindow(gui->display, vi->screen), 
			   0, 0, width, height, 0, vi->depth, InputOutput, 
			   vi->visual, 
			   CWBorderPixel|CWColormap|CWEventMask, &swa);
  XFree(vi);
  if(!gui->win) {
    return false;
  }

  //TODO: Do this only once
#ifdef GUI_DRAG_N_DROP
  /////// Drag And Drop
  // Define atoms
  XdndAware = XInternAtom(gui->display, "XdndAware", False);
  XA_ATOM = XInternAtom(gui->display, "XA_ATOM", False);
  XdndEnter = XInternAtom(gui->display, "XdndEnter", False);
  XdndPosition = XInternAtom(gui->display, "XdndPosition", False);
  XdndActionCopy = XInternAtom(gui->display, "XdndActionCopy", False);
  XdndLeave = XInternAtom(gui->display, "XdndLeave", False);
  XdndStatus = XInternAtom(gui->display, "XdndStatus", False);
  XdndDrop = XInternAtom(gui->display, "XdndDrop", False);
  XdndSelection = XInternAtom(gui->display, "XdndSelection", False);
  XDND_DATA = XInternAtom(gui->display, "XDND_DATA", False);
  XdndTypeList = XInternAtom(gui->display, "XdndTypeList", False);
  XdndFinished = XInternAtom(gui->display, "XdndFinished", False);
  WM_PROTOCOLS = XInternAtom(gui->display, "WM_PROTOCOLS", False);
  WM_DELETE_WINDOW = XInternAtom(gui->display, "WM_DELETE_WINDOW", False);

  Atom version = 5;
  XChangeProperty(gui->display, gui->win, XdndAware, XA_ATOM, 32, PropModeReplace,
		  (unsigned char *)&version, 1);

  Atom xdndActionListAtom = XInternAtom(gui->display, "XdndActionList", False);
  Atom xdndActionCopyAtom = XInternAtom(gui->display, "XdndActionCopy", False);
  Atom actions[] = { xdndActionCopyAtom };
  XChangeProperty(gui->display, gui->win, xdndActionListAtom, XA_ATOM, 32, PropModeReplace,
		  (unsigned char *)actions, 1);

  // Define type atoms we will accept for file drop
  typesWeAccept[0] = XInternAtom(gui->display, "text/uri-list", False);
  typesWeAccept[1] = XInternAtom(gui->display, "UTF8_STRING", False);
  typesWeAccept[2] = XInternAtom(gui->display, "TEXT", False);
  typesWeAccept[3] = XInternAtom(gui->display, "STRING", False);
  typesWeAccept[4] = XInternAtom(gui->display, "text/plain;charset=utf-8", False);
  typesWeAccept[5] = XInternAtom(gui->display, "text/plain", False);
#endif //GUI_DRAG_N_DROP

  Atom wmDeleteMessage = XInternAtom(gui->display, "WM_DELETE_WINDOW", False);
  Atom sel = XInternAtom(gui->display, "CLIPBOARD", False);
  Atom utf8 = XInternAtom(gui->display, "UTF8_STRING", False);
  Window owner = XGetSelectionOwner(gui->display, sel);
  if(owner == None) {
    printf("Clibpboard has no owner");
    return false;
  }

  gui->wmDeleteMessage = (int) wmDeleteMessage;
  gui->clipboardAtom = (int) sel;
  XSetWMProtocols(gui->display, gui->win, &wmDeleteMessage, 1);
  XStoreName(gui->display, gui->win, name);
  XSelectInput(gui->display, gui->win,
	       ButtonPressMask | ButtonReleaseMask |
	       ExposureMask |
	       PointerMotionMask |
	       EnterWindowMask | LeaveWindowMask |
	       PointerMotionMask |
#ifdef GUI_DRAG_N_DROP	       
	       EnterWindowMask | LeaveWindowMask |
#endif //GUI_DRAG_N_DROP	       
	       KeyPressMask | KeyReleaseMask | StructureNotifyMask);
  XMapWindow(gui->display, gui->win);

  gui->ctx = glXCreateNewContext(gui->display, bestFbc, GLX_RGBA_TYPE, 0, True );
  XSync(gui->display, False);  
  glXMakeCurrent(gui->display, gui->win, gui->ctx);

  gui->running = true;

  /////////////////////////////////////////

  gui->fd = -1; //ConnectionNumber(gui->display);  
  gui->tv.tv_usec = 0;
  gui->tv.tv_sec = 1;
  
#ifdef GUI_DRAG_N_DROP
  gui->clean_up_for_xdnd = false;
  memset(&xdndState, 0, sizeof(xdndState));
#endif //GUI_DRAG_N_DROP  
  
  XConvertSelection(gui->display, sel, utf8, sel, gui->win, CurrentTime);

  return true;
}

#ifdef GUI_DRAG_N_DROP

//TODO: maybe you should implement an abstraction
#include <stdlib.h>

GUI_DEF char *gui_xdnd_get_copied_data(Display *disp, Window source) {
  // Declare return value
  char *retVal = NULL;

  // Try to get PRIMARY property
  Atom actualType = None;
  int actualFormat;
  unsigned long numOfItems, bytesAfterReturn;
  unsigned char *data = NULL;
  if (XGetWindowProperty(disp, source, XDND_DATA, 0, 1024, False, AnyPropertyType,
			 &actualType, &actualFormat, &numOfItems, &bytesAfterReturn, &data) == Success) {

    printf("numOfItems: %lu, bytesAfterReturn: %lu\n", numOfItems, bytesAfterReturn);
    printf("'%.*s'\n", (int) numOfItems, data);
      
    // Allocate temporary buffer
    char *tempBuffer = malloc(numOfItems + 1);
    if (!tempBuffer) {
      //TODO: log error
    }

    // Copy all data from X buffer then add null-byte to create proper string, then
    // dispose of X buffer
    memcpy(tempBuffer, data, numOfItems);
    tempBuffer[numOfItems] = '\0';
    XFree(data);

    // Copy from beyond 'file://' prefix if present
    char *tempPtr;
    if ((tempPtr = strstr(tempBuffer, "file://")) != NULL) {
      tempPtr = tempBuffer + 7;
    } else {
      tempPtr = tempBuffer;
    }

    // Check if cr/nl ending is present and terminate string
    // before this if so
    if (tempPtr[strlen(tempPtr)-2] == 0xD && tempPtr[strlen(tempPtr)-1] == 0xA)
      tempPtr[strlen(tempPtr)-2] = '\0';

    // Allocate return buffer
    retVal = malloc(strlen(tempPtr) + 1);
    if (!retVal) {
      //TODO: log error
    }

    // Copy data from temp buffer to it, then free temp buffer
    memcpy(retVal, tempPtr, strlen(tempPtr));
    retVal[strlen(tempPtr)] = '\0';
    free(tempBuffer);
  }

  // Return malloc allocated buffer - caller must free
  return retVal;
}

GUI_DEF void gui_xdnd_send_status(Display *disp, Window source, Window target, Atom action)
{
  if (xdndState.xdndExchangeStarted && !xdndState.amISource) {
    // Declare message struct and populate its values
    XEvent message;
    memset(&message, 0, sizeof(message));
    message.xclient.type = ClientMessage;
    message.xclient.display = disp;
    message.xclient.window = target;
    message.xclient.message_type = XdndStatus;
    message.xclient.format = 32;
    message.xclient.data.l[0] = source;
    message.xclient.data.l[1] = 1; // Sets accept and want position flags

    // Send back window rectangle coordinates and width
    message.xclient.data.l[2] = 0;
    message.xclient.data.l[3] = 0;

    // Specify action we accept
    message.xclient.data.l[4] = action;

    // Send it to target window
    if (XSendEvent(disp, target, False, 0, &message) == 0) {
      //TODO: log error
    }
  }
}

GUI_DEF void gui_xdnd_send_leave(Display *disp, Window source, Window target)
{
  if (xdndState.xdndExchangeStarted && xdndState.amISource) {
    // Declare message struct and populate its values
    XEvent message;
    memset(&message, 0, sizeof(message));
    message.xclient.type = ClientMessage;
    message.xclient.display = disp;
    message.xclient.window = target;
    message.xclient.message_type = XdndLeave;
    message.xclient.format = 32;
    message.xclient.data.l[0] = source;
    // Rest of array members reserved so not set

    // Send it to target window
    if (XSendEvent(disp, target, False, 0, &message) == 0) {
      //TODO: log error
    }
  }
}

GUI_DEF bool gui_xdnd_do_we_accept(Atom a) {
  for (unsigned int i = 0; i < sizeof(typesWeAccept) / sizeof(Atom); ++i) {
    if (a == typesWeAccept[i]) {
      return true;
    }
  }

  return false;
}

GUI_DEF Atom gui_xdnd_get_supported_type(Display *disp, Window source) {
  // Try to get XdndTypeList property
  Atom retVal = None;
  Atom actualType = None;
  int actualFormat;
  unsigned long numOfItems, bytesAfterReturn;
  unsigned char *data = NULL;
  if (XGetWindowProperty(disp, source, XdndTypeList, 0, 1024, False, AnyPropertyType,
			 &actualType, &actualFormat, &numOfItems, &bytesAfterReturn, &data) == Success) {
    if (actualType != None) {
      Atom *supportedAtoms = (Atom *)data;
      for (unsigned int i = 0; i < numOfItems; ++i) {
	if (gui_xdnd_do_we_accept(supportedAtoms[i])) {
	  retVal = supportedAtoms[i];
	  break;
	}
      }
			
      XFree(data);
    }
  }

  return retVal;
}

GUI_DEF void gui_xdnd_send_finished(Display *disp, Window source, Window target)
{
  if (xdndState.xdndExchangeStarted && !xdndState.amISource) {
    // Declare message struct and populate its values
    XEvent message;
    memset(&message, 0, sizeof(message));
    message.xclient.type = ClientMessage;
    message.xclient.display = disp;
    message.xclient.window = target;
    message.xclient.message_type = XdndFinished;
    message.xclient.format = 32;
    message.xclient.data.l[0] = source;
    message.xclient.data.l[1] = 1;
    message.xclient.data.l[2] = XdndActionCopy;

    // Send it to target window
    if (XSendEvent(disp, target, False, 0, &message) == 0) {
      //TODO: log error
    }      
  }
}

#endif //GUI_DRAG_N_DROP

GUI_DEF bool gui_peek(Gui *gui, Gui_Event *event) {

#ifdef GUI_DRAG_N_DROP
  if(gui->clean_up_for_xdnd) {
    XDeleteProperty(gui->display, gui->win, XDND_DATA);
    gui_xdnd_send_finished(gui->display, gui->win, xdndState.otherWindow);
    memset(&xdndState, 0, sizeof(xdndState));
    gui->clean_up_for_xdnd = false;
  }
#endif //GUI_DRAG_N_DROP

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
#ifdef GUI_DRAG_N_DROP
      if(!xdndState.xdndExchangeStarted) {
	if (e->xclient.message_type == XdndEnter) {
	  // Update state
	  xdndState.xdndExchangeStarted = true;
	  xdndState.amISource = false;
	  xdndState.otherWindow = e->xclient.data.l[0];

	  // Determine type to ask for
	  if (e->xclient.data.l[1] & 0x1) {
	    // More than three types, look in XdndTypeList
	    xdndState.proposedType =
	      gui_xdnd_get_supported_type(gui->display, xdndState.otherWindow);
	  } else {
	    // Only three types, check three in turn and stop when we find
	    // one we support
	    xdndState.proposedType = None;
	    for (int i = 2; i < 5; ++i) {
	      if (gui_xdnd_do_we_accept(e->xclient.data.l[i])) {
		xdndState.proposedType = e->xclient.data.l[i];
		break;
	      }
	    }
	  }
	}	
      } else {
	if(xdndState.amISource) {
	  if (e->xclient.message_type == XdndStatus) {
	    xdndState.xdndStatusReceived = true;

	    // Check if target will accept drop
	    if ((e->xclient.data.l[1] & 0x1) != 1) {
	      // Won't accept, break exchange and wipe state
	      printf("XDND: sending XdndLeave message to target window "
		     "as it won't accept drop\n");
	      gui_xdnd_send_leave(gui->display, gui->win, xdndState.otherWindow);
	      memset(&xdndState, 0, sizeof(xdndState));
	      //break; //DO something about that 
	    }
	  }
	  else if (e->xclient.message_type == XdndFinished) {
	    memset(&xdndState, 0, sizeof(xdndState));	    
	    /* square.visible = false; */
	    /* drawSquare(disp, wind, gContext, &square); */
	  }
	} else {
	  // Check for XdndPosition message
	  if (e->xclient.message_type == XdndPosition) {
	    // Ignore if not for our window and sent erroneously
	    if (xdndState.xdndPositionReceived &&
		(Window) e->xclient.data.l[0] != xdndState.otherWindow) { //TODO: is this correct ?
	      printf("XDND: receiving XdndPosition from erroneous "
		     "window, ignoring\n");
	      //break; //TODO: do something about that
	    }

	    // Update state
	    xdndState.xdndPositionReceived = true;
	    xdndState.p_rootX = e->xclient.data.l[2] >> 16;
	    xdndState.p_rootY = e->xclient.data.l[2] & 0xFFFF;
	    xdndState.proposedAction = e->xclient.data.l[4];
	    xdndState.xdndLastPositionTimestamp = e->xclient.data.l[3];

	    // Now check if we should send XdndStatus message
	    if (!xdndState.xdndStatusSent) {
	      xdndState.xdndStatusSent = true;
	      gui_xdnd_send_status(gui->display, gui->win,
				   xdndState.otherWindow, xdndState.proposedAction);
	    }
	  }

	  // Check for XdndLeave message
	  if (e->xclient.message_type == XdndLeave) {
	    memset(&xdndState, 0, sizeof(xdndState));
	  }

	  // Check for XdndDrop message
	  if (e->xclient.message_type == XdndDrop) {
	    if (e->xselection.property == None) {
	      //printf("xselection_property is None\n");
	    }

	    // Ignore if not for our window and/or sent erroneously
	    if (!xdndState.xdndPositionReceived ||
		(Window) e->xclient.data.l[0] != xdndState.otherWindow) { //TODO: is this correct?
	      printf("XDND: receiving XdndDrop from erroneous "
		     "window, ignoring\n");
	      //break; //Do something about that
	    }

	    // Update state
	    xdndState.xdndDropReceived = true;
	    xdndState.xdndDropTimestamp = e->xclient.data.l[2];
	    
	    // Call XConvertSelection
	    XConvertSelection(gui->display, XdndSelection, xdndState.proposedType,
			      XDND_DATA, gui->win, xdndState.xdndDropTimestamp);
	  }
	}
      }
    } else if(e->type == SelectionRequest) {
      if (xdndState.xdndExchangeStarted && xdndState.amISource) {
	// Add data to the target window
	// sendSelectionNotify(disp, &event.xselectionrequest,
	//   saveSquareState(&square)); ???
      }
    } else if(e->type == SelectionNotify) {
      if (e->xselection.property == XDND_DATA) {
	gui->clean_up_for_xdnd = true;
	event->as.value = (long long) (void *) gui;
	event->type = GUI_EVENT_FILEDROP;
      }
#endif //GUI_DRAG_N_DROP      
    } else if(e->type == ButtonPress ||
	      e->type == ButtonRelease) {
      event->type = e->type == ButtonPress ?
	GUI_EVENT_MOUSEPRESS : GUI_EVENT_MOUSERELEASE;
      
      if(e->xbutton.button == Button1) {
	event->as.key = 'L';
      } else if(e->xbutton.button == Button2) {
	event->as.key = 'M';
      } else if(e->xbutton.button == Button3) {
	event->as.key = 'R';
      }

      if(e->xbutton.button == Button4) { //TO top
	event->type = GUI_EVENT_MOUSEWHEEL;
	event->as.amount = 1;
      } else if(e->xbutton.button == Button5) { //TO bottom
	event->type = GUI_EVENT_MOUSEWHEEL;
	event->as.amount = -1;
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
      event->as.key = XkbKeycodeToKeysym(gui->display, e->xkey.keycode, 0, 1);
    }
    return event->type != GUI_EVENT_NONE;
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


GUI_DEF bool gui_dragged_files_init(Gui_Dragged_Files *files, Gui_Event *event) {
#ifdef GUI_DRAG_N_DROP
  Gui *gui = (Gui *) (void *) event->as.value;
  Display* disp = gui->display;
  Window source = gui->win;
  
  // Try to get PRIMARY property
  Atom actualType = None;
  int actualFormat;
  files->data = NULL;
  if (XGetWindowProperty(disp, source, XDND_DATA, 0, 1024, False, AnyPropertyType,
			 &actualType, &actualFormat, &files->numOfItems, &files->bytesAfterReturn, &files->data) != Success) {
    return false;
  }

  if(files->numOfItems == 0) {
    return false;
  }
  
  return true;
#else
  (void) files;
  (void) event;
  return false;
#endif //GUI_DRAG_N_DROP
}

GUI_DEF bool gui_dragged_files_next(Gui_Dragged_Files *files, char **path) {
#ifdef GUI_DRAG_N_DROP
  const char *prefix = "file://";
  size_t prefix_len = strlen(prefix);

  int off = -1;
  for(size_t i=0;i<files->numOfItems - prefix_len;i++) {
    size_t j = 0;
    for(;j<prefix_len;j++) {
      if(files->data[i+j] != prefix[j]) {
	break;
      }
    }
    if(j == prefix_len) {
      off = (int) i;
      break;
    }
  }
  char *start = NULL;
  if(off != -1) {
    start = ((char *) files->data) + off;
  }
    
  if(start == NULL) {    
    return false;
  } else {
    files->data = (unsigned char *) start + prefix_len;
    size_t len = 0;
#include <ctype.h>
    while( files->data[len] != 0 && isprint(files->data[len]) ) len++;
    memcpy(files->path, files->data, len);
    files->path[len] = '\0';
    *path = files->path;
    files->data = files->data+ len;
    files->numOfItems -= (size_t) off + len;    
    return true;
  }
#else
  (void) files;
  (void) path;
  return false;
#endif //GUI_DRAG_N_DROP
}

GUI_DEF void gui_dragged_files_free(Gui_Dragged_Files *files) {
  (void) files;
}

//TODO
GUI_DEF bool gui_open_file_dialog(char *buffer, size_t buffer_cap) {
  (void) buffer;
  (void) buffer_cap;
  return false;
}

//TODO
GUI_DEF bool gui_clipboard_init(Gui_Clipboard *clipboard, Gui *gui, char **text) {
  (void) clipboard;
  (void) text;

  Atom da, incr, type;
  int di;
  unsigned long size, dul;
  unsigned char *prop_ret = NULL;

  /* Dummy call to get type and size. */
  XGetWindowProperty(gui->display, gui->win, gui->clipboardAtom, 0, 0, False, AnyPropertyType,
		     &type, &di, &dul, &size, &prop_ret);
  XFree(prop_ret);

  incr = XInternAtom(gui->display, "INCR", False);
  if (type == incr)
    {
      printf("Data too large and INCR mechanism not implemented\n");
      return false;
    }

  /* Read the data in one go. */
  printf("Property size: %lu\n", size);

  XGetWindowProperty(gui->display, gui->win, gui->clipboardAtom, 0, size, False, AnyPropertyType,
		     &da, &di, &dul, &dul, &prop_ret);
  printf("%s", prop_ret);
  fflush(stdout);
  XFree(prop_ret);

  /* Signal the selection owner that we have successfully read the
   * data. */
  XDeleteProperty(gui->display, gui->win, gui->clipboardAtom);
  return true;
}

//TODO
GUI_DEF void gui_clipboard_free(Gui_Clipboard *clipboard) {
  (void) clipboard;
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

#ifndef UsedWindowProc
#define UsedWindowProc DefWindowProc
#endif //UsedWindowProc

//DefWindowProc vs LSMProc
LRESULT CALLBACK Gui_Implementation_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if(message == WM_COPYDATA) {
      
    //TODO: maybe introduce callbacks ?
    COPYDATASTRUCT *cds = (COPYDATASTRUCT *) lParam;
    Gui *gui = (Gui *) GetWindowLongPtr(hWnd, 0);
    if(gui != NULL) {
      if(gui->data.data) {
	free(gui->data.data);
      }
      gui->data.data = (unsigned char *) malloc(cds->cbData);
      if(gui->data.data) {
	gui->has_data = true;		
	gui->data.size = cds->cbData;
	memcpy(gui->data.data, cds->lpData, cds->cbData);
      }
    }	
    return gui != NULL;
  } else if(message == WM_CLOSE) {
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
    return UsedWindowProc(hWnd, message, wParam, lParam);
  } else {
    return UsedWindowProc(hWnd, message, wParam, lParam);
  }
}

GUI_DEF bool gui_init(Gui *gui, Gui_Canvas *canvas, const char *name) {

#ifndef GUI_CONSOLE
  FreeConsole();
#endif //GUI_CONSOLE
  SetProcessDPIAware();
  
  if(!guiWin32PerfCountFrequency.QuadPart) {
      QueryPerformanceFrequency(&guiWin32PerfCountFrequency);
  }

  HMODULE hInstance = GetModuleHandle(NULL);

  STARTUPINFO startupInfo;
  GetStartupInfo(&startupInfo);
  DWORD nCmdShow = startupInfo.wShowWindow;

  WNDCLASSEX wc = {0};
  wc.cbSize = sizeof(WNDCLASSEX);
  //wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = Gui_Implementation_WndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = name;
  wc.cbWndExtra = sizeof(LONG_PTR);
  wc.cbClsExtra = 0;
  //HANDLE icon = LoadIcon(NULL, IDI_APPLICATION);
  /*
    HANDLE icon = (HICON) LoadImage( // returns a HANDLE so we have to cast to HICON
    NULL,             // hInstance must be NULL when loading from a file
    "rsc\\icon.ico",   // the icon file name
    IMAGE_ICON,       // specifies that the file is an icon
    0,                // width of the image (we'll specify default later on)
    0,                // height of the image
    LR_LOADFROMFILE|  // we want to load a file (as opposed to a resource)
    LR_DEFAULTSIZE|   // default metrics based on the type (IMAGE_ICON, 32x32)
    LR_SHARED         // let the system release the handle when it's no longer used
    );
  */

  /*
  //TODO: unhardcode this
  int offset = LookupIconIdFromDirectoryEx(icon_data, TRUE, 0, 0, LR_DEFAULTCOLOR);
  HICON icon = CreateIconFromResourceEx(icon_data + offset, icon_size - offset, TRUE, 0x30000, 0, 0, LR_DEFAULTCOLOR);
  */
  HICON icon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
  wc.hIcon = icon; // ICON when tabbing
  wc.hIconSm = icon; //ICON default
  wc.hCursor = LoadCursor (NULL, IDC_ARROW);
  //wc.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);

  if(!RegisterClassEx(&wc)) {
    return false;
  }

  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  DWORD flags = 0;
#ifdef GUI_DRAG_N_DROP
#  ifdef _MSC_VER
#    pragma comment(lib,"shell32.lib")
#  endif //_MSC_VER
  flags |= WS_EX_ACCEPTFILES;
#endif //GUI_DRAG_N_DROP

  gui->win = CreateWindowEx(flags,
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
			    NULL);
  if(!gui->win) {
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
    gui_use_vsync(1);
  }

  if(canvas != NULL) {
    gui->canvas = canvas;
  } else {
    gui->canvas = (Gui_Canvas *)(void *) gui;
  }

  gui->running = true;
  gui->has_data = false;
  gui->data.data = NULL;

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
      return true;
    }
  }    

  bool result = PeekMessage(msg, gui->win, 0, 0, PM_REMOVE);
  if(!result) {

    if(gui->has_data) {
      gui->has_data = false;	    
      event->type = GUI_EVENT_DATARECEIVE;
      event->as.data = gui->data;
      return true;
    }
	
    return result;
  }
  TranslateMessage(msg);
#ifdef GUI_LOOPLESS
  SizingCheck(msg);
#endif //GUI_LOOPLESS
  DispatchMessage(msg);

  switch(msg->message) {
  case WM_DROPFILES: {
    event->type = GUI_EVENT_FILEDROP;
    event->as.value = msg->wParam;
  } break;
  case WM_MOUSEWHEEL:
  case WM_MOUSEHWHEEL: {
    event->type = GUI_EVENT_MOUSEWHEEL; 
    event->as.amount = GET_WHEEL_DELTA_WPARAM(msg->wParam) / 120;//apperantly this is 120`s steps
  } break;
  case WM_RBUTTONUP: 
  case WM_RBUTTONDOWN: {
    event->type = GUI_EVENT_MOUSEPRESS;
    if(msg->message == WM_RBUTTONUP) {
      event->type = GUI_EVENT_MOUSERELEASE;	  
    }
    event->as.key = 'R';
  } break;
  case WM_LBUTTONUP: 
  case WM_LBUTTONDOWN: {
    event->type = GUI_EVENT_MOUSEPRESS;
    if(msg->message == WM_LBUTTONUP) {
      event->type = GUI_EVENT_MOUSERELEASE;	  
    }
    event->as.key = 'L';
  } break;
  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP:
  case WM_KEYDOWN:
  case WM_KEYUP: {
    bool wasDown = ((msg->lParam & (1 << 30)) != 0);
    bool isDown = ((msg->lParam & (1 << 31)) == 0);

    if(wasDown != isDown) {
      event->as.key = (char) msg->wParam;
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

  return event->type != GUI_EVENT_NONE;
}

GUI_DEF void gui_time_capture(Gui_Time *time) {
  QueryPerformanceCounter(&time->counter);
}

GUI_DEF unsigned long gui_time_measure(Gui_Time *reference) {
  LARGE_INTEGER endCounter;
  QueryPerformanceCounter(&endCounter);

  long elapsed = (long) (endCounter.QuadPart - reference->counter.QuadPart);
  reference->counter = endCounter;
  return (unsigned long) ((1000 * elapsed) / guiWin32PerfCountFrequency.QuadPart);
}

GUI_DEF bool gui_init_opengl(Gui *gui) {
#ifndef GUI_NO_OPENGL
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
#endif //GUI_NO_OPENGL
  return true;
}

GUI_DEF bool gui_use_vsync(int use) {
    return wglSwapIntervalEXT(use);
}

GUI_DEF bool gui_free(Gui *gui) {
  ReleaseDC(gui->win, gui->dc);
  return DestroyWindow(gui->win);
}

GUI_DEF bool gui_dragged_files_init(Gui_Dragged_Files *files, Gui_Event *event) {
  files->h_drop = (HDROP) event->as.value;
  files->count = DragQueryFile(files->h_drop, 0xffffffff, files->path, MAX_PATH);
  if(files->count <= 0) {
    return false;
  }
  files->index = 0;

  return true;
}

GUI_DEF bool gui_dragged_files_next(Gui_Dragged_Files *files, char **path) {
  if(files->index >= files->count) {
    return false;
  }
  DragQueryFile(files->h_drop, files->index++, files->path, MAX_PATH);
  *path = files->path;

  return true;
}

GUI_DEF void gui_dragged_files_free(Gui_Dragged_Files *files) {
  DragFinish(files->h_drop);
}

//UNHARDCODE filter
GUI_DEF bool gui_open_file_dialog(char *buffer, size_t buffer_cap) {
  OPENFILENAME of = {0};
  of.lStructSize = sizeof(OPENFILENAME);
  of.hwndOwner = NULL;
  of.lpstrFile = buffer;
  of.lpstrFile[0] = '\0';
  of.nMaxFile = (DWORD) buffer_cap;
  of.lpstrFilter = "Image\0*.PNG;*.JPG\0All\0*.*\0";
  of.nFilterIndex = 1;
  of.lpstrFileTitle = NULL ;
  of.nMaxFileTitle = 0 ;
  of.lpstrInitialDir= NULL ;
  of.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;
  
  return GetOpenFileName(&of);
}

GUI_DEF bool gui_clipboard_init(Gui_Clipboard *clipboard, Gui *gui, char **text) {
  (void) gui;
  if (!OpenClipboard(NULL)) {
    return false;
  }
  
  clipboard->handle = GetClipboardData(CF_TEXT);
  if(clipboard->handle == NULL) {
    CloseClipboard();
    return false;
  }

  *text = GlobalLock(clipboard->handle);
  if((*text) == NULL) {
    CloseClipboard();
    return false;
  }  
  
  return true;
}

GUI_DEF void gui_clipboard_free(Gui_Clipboard *clipboard) {
  GlobalUnlock(clipboard->handle);
  CloseClipboard();
}

GUI_DEF void gui_swap_buffers(Gui *gui) {
  SwapBuffers(gui->dc);
}

#endif //_WIN32

GUI_DEF const char *gui_shader_type_as_cstr(GLuint shader) {
    switch (shader) {
    case GL_VERTEX_SHADER:
        return "GL_VERTEX_SHADER";
    case GL_FRAGMENT_SHADER:
        return "GL_FRAGMENT_SHADER";
    default:
        return "(Unknown)";
    }
}

GUI_DEF bool gui_compile_shader(GLuint *shader, GLenum shaderType, const char *shader_source) {
    *shader  =glCreateShader(shaderType);
    glShaderSource(*shader, 1, &shader_source, NULL);
    glCompileShader(*shader);

    GLint compiled = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
	/*
	GLchar message[1024];
	GLsizei message_size = 0;
	glGetShaderInfoLog(*shader, sizeof(message), &message_size, message);
	fprintf(stderr, "ERROR: could not compile %s\n", shader_type_as_cstr(shaderType));
	fprintf(stderr, "%.*s\n", message_size, message);
	*/
	return false;
    }

    return true;

}

GUI_DEF bool gui_link_program(GLuint *program, GLuint vertex_shader, GLuint fragment_shader) {
    *program = glCreateProgram();
    glAttachShader(*program, vertex_shader);
    glAttachShader(*program, fragment_shader);

    glLinkProgram(*program);
  
    GLint linked = 0;
    glGetProgramiv(*program, GL_LINK_STATUS, &linked);
    if(!linked) {
	/*
	GLsizei message_size = 0;
	GLchar message[1024];

	glGetProgramInfoLog(*program, sizeof(message), &message_size, message);
	fprintf(stderr, "ERROR: Program Linking: %.*s\n", message_size, message);
	*/
	return false;
    }
  
    return true;
    
}

#endif //GUI_IMPLEMENTATION

#endif //GUI_H_H

