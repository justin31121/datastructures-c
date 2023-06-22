#include <stdio.h>

#define GUI_IMPLEMENTATION
#define GUI_DRAG_N_DROP
#define GUI_CONSOLE
#include "../libs/gui.h"

#define IO_IMPLEMENTATION
#include "../libs/io.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

bool rect_intersect(RECT a, RECT b) {

    return (a.right > b.left) &&
	(a.bottom > b.top) &&
	(b.right > a.left) &&
	(b.bottom > a.top);
}

bool is_window_visible(HWND hwnd) {
    char buffer[MAX_PATH];

    RECT hwnd_rect;
    if(!GetWindowRect(hwnd, &hwnd_rect)) {
	//SHOULD NOT HAPPEN
	return false;
    }

    HWND window = GetTopWindow(GetDesktopWindow());
    do{
	if(window == hwnd) {
	    return true;
	}
	
	if(IsWindowVisible(window) && (GetWindowLong(window, GWL_STYLE) & WS_EX_APPWINDOW)
	   && !IsIconic(window) ) {
	    int n = GetWindowTextA(window, buffer, sizeof(buffer));
	    if(n != 0) {

		RECT rect;
		if(GetWindowRect(window, &rect)) {
		    if(rect_intersect(hwnd_rect, rect)) {
			return false;
		    }
		}
		
	    }
	}
	
	window = GetWindow(window, GW_HWNDNEXT);
    }while(window);

    return true;
}

#include <psapi.h>

GLuint textures;
bool file_uploaded = false;

unsigned char *img_data = NULL;
float img_width = 0;
float img_height = 0;
    
float z;

void maybe_load_file(const char *path) {
    bool is_file;

    if(!io_exists(path, &is_file) || !is_file) {
	fprintf(stderr, "ERROR: This is not a valid file: '%s'\n", path);
	return;
    }

    int img_width_int, img_height_int;
    img_data = stbi_load(path, &img_width_int, &img_height_int, 0, 4);
    if(!img_data) {
	fprintf(stderr, "ERROR: Can not load file: '%s'\n", path);
	return;
    }
    img_width = (float) img_width_int;
    img_height = (float) img_height_int;

    if(file_uploaded) {
	glDeleteTextures(1, &textures);
    }
  
    glGenTextures(1, &textures);
    glBindTexture(GL_TEXTURE_2D, textures);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D,
		 0,
		 GL_RGBA,
		 img_width_int,
		 img_height_int,
		 0,
		 GL_RGBA,
		 GL_UNSIGNED_BYTE,
		 img_data);
    stbi_image_free(img_data);			   
    file_uploaded = true;
    z = 20.f;    
}

void clampf(float *f, float min, float max) {
    if(*f > max) *f = max;
    if(*f < min) *f = min;
}

int main(int argc, char ** argv) {

#ifdef _WIN32
  //TODO: Implement named pipe Inter-Proccess-Communication(ICP)
  //Example: https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/ipc/transactions-on-named-pipes.md
    
  DWORD processes[4096];
  DWORD bytesGot;
  EnumProcesses(processes, sizeof processes, &bytesGot);
  DWORD processesCount = bytesGot / sizeof(DWORD);

  char currName[MAX_PATH];
  GetModuleFileNameEx(GetCurrentProcess(), NULL, currName, MAX_PATH);
  DWORD currProcessId = GetCurrentProcessId();

  for(unsigned int x=0;x<processesCount;x++) {
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[x]);
    char name[MAX_PATH];
    GetModuleFileNameEx(process, NULL, name, MAX_PATH);
    //printf("\tname: %s (%d)\n", name, processes[x]);
    if(processes[x] != currProcessId && strcmp(name, currName) == 0) {
      FreeConsole();
      if(argc > 1) {
	size_t len = strlen(argv[1]);
				
	COPYDATASTRUCT cds = {0};
	cds.dwData = 0;
	cds.cbData = (DWORD) len + 1;
	cds.lpData = argv[1];

	HWND hCurWnd = NULL;
	do {
	  hCurWnd = FindWindowEx(NULL, hCurWnd, NULL, NULL);
	  if(hCurWnd == NULL) break;
	  
	  DWORD checkProcessID = 0;
	  GetWindowThreadProcessId(hCurWnd, &checkProcessID);
	  if (checkProcessID == processes[x]) {
	    SendMessage(hCurWnd, WM_COPYDATA, (WPARAM) 0, (LPARAM) &cds);	    
	  }
	  
	} while (1);
		
      }
      return 0;
    }
  }

#endif //_WIN32

    
    /////////////////////////////////////////////////////////
    z = 1.f;

    Gui gui;
    Gui_Canvas canvas = {600, 600, NULL};
    if(!gui_init(&gui, &canvas, "viewer")) {
	return 1;
    }
    gui_use_vsync(1);


    if(argc > 1) {
	//maybe_load_ico();
	maybe_load_file(argv[1]);
    }

    float width = 0;
    float height = 0;
    Gui_Event event;
    while(gui.running) {
	while(gui_peek(&gui, &event)) {
	    if(event.type == GUI_EVENT_KEYPRESS) {		
		if(event.as.key == 'Q') {
		    gui.running = false;
		} else if(event.as.key == 'P') {

#ifdef _MSC_VER
#pragma comment(lib,"comdlg32.lib")
#endif //_MSVC_
		  char path[MAX_PATH];
		  if(gui_open_file_dialog(path, sizeof(path))) {
		    maybe_load_file(path);
		  }		  

		} else if(event.as.key == 'V') {

		  char *text;

		  Gui_Clipboard clipboard;
		  if(gui_clipboard_init(&clipboard, &text)) {
		    printf("'%s'", text); fflush(stdout);
		    gui_clipboard_free(&clipboard);
		  }
		  
	    } else if(event.type == GUI_EVENT_FILEDROP) {

		char *path;
		Gui_Dragged_Files files;
		if(gui_dragged_files_init(&files, &event)) {
		    if(gui_dragged_files_next(&files, &path)) {
			maybe_load_file(path);
		    }

		    gui_dragged_files_free(&files);
		}
	    } else if(event.type == GUI_EVENT_MOUSEWHEEL) {
		z -= (float) event.as.amount;
		clampf(&z, 1.f, 20.f);
	    } else if(event.type == GUI_EVENT_DATARECEIVE) {
	      
		const char *filepath = (char *) event.as.data.data;
		maybe_load_file(filepath);

		//TODO: add is visible functioniality
		if( IsIconic(gui.win) || !is_window_visible(gui.win)) {
		    if(GetActiveWindow() != gui.win) {
			SetActiveWindow(gui.win);			
		    }
		    SetForegroundWindow(gui.win);		    
		}		
	    }
	}

	gui_get_window_sizef(&gui, &width, &height);
	glViewport(0, 0, (int) width, (int) height);
	glClearColor(0.2f, 0.2f, 0.2f, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	if(file_uploaded) {

	    glEnable(GL_TEXTURE_2D);

	    //float zoom = z * (high - low) / 20.f + low;
	    float zoom = z / 20.f;
	    
	    //printf("z: %f, zoom: %f\n", z, zoom); fflush(stdout);
	    
	    float x1 = 0;
	    float x2 = 0;
	    float y1 = 0;
	    float y2 = 0;

	    bool a = img_width > width;
	    bool b = img_height > height;

	    float ratio =  img_width / img_height;

	    if(!a && !b) {
		x1 = ((width - img_width) / 2.f) / width;
		x2 = ((width - img_width) / 2.f + img_width) / width;
		y1 = ((height - img_height) / 2.f) / height;
		y2 = ((height - img_height) / 2.f + img_height) / height;	
	    } else {		
		float _img_height = width / ratio;
		if(_img_height < height) {
		    x1 = 0;
		    x2 = 1;
		    y1 = ((height - _img_height) / 2.f) / height;
		    y2 = ((height - _img_height) / 2.f + _img_height) / height;
		} else {
		    float _img_width = ratio * height;
		    y1 = 0;
		    y2 = 1;
		    x1 = ((width - _img_width) / 2.f) / width;
		    x2 = ((width - _img_width) / 2.f + _img_width) / width;
		}	      

	    }

	    x1 = x1 * 2.f - 1.f;
	    x2 = x2 * 2.f - 1.f;
	    y1 = y1 * 2.f - 1.f;
	    y2 = y2 * 2.f - 1.f;

	    x1 /= zoom;
	    x2 /= zoom;
	    y1 /= zoom;
	    y2 /= zoom;
	    
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	    glEnable(GL_BLEND);

	    glBegin(GL_QUADS);
	    glTexCoord2f(0, 0); glVertex2f(x1, y2);
	    glTexCoord2f(1, 0); glVertex2f(x2, y2);
	    glTexCoord2f(1, 1); glVertex2f(x2, y1);
	    glTexCoord2f(0, 1); glVertex2f(x1, y1);
	    glEnd();			
	}

	gui_swap_buffers(&gui);
    }
    
    return 0;
}

