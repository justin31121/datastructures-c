#define GUI_IMPLEMENTATION
#define GUI_DRAG_N_DROP
#include "../libs/gui.h"

#define IO_IMPLEMENTATION
#include "../libs/io.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

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

HWND GetAllWindowsFromProcessID(DWORD dwProcessID)
{
    // find all hWnds (vhWnds) associated with a process id (dwProcessID)
    HWND last = NULL;
    HWND hCurWnd = NULL;
    do
    {
        hCurWnd = FindWindowEx(NULL, hCurWnd, NULL, NULL);
        DWORD checkProcessID = 0;
        GetWindowThreadProcessId(hCurWnd, &checkProcessID);
        if (checkProcessID == dwProcessID)
        {
	    last = hCurWnd;
        }
    }
    while (hCurWnd != NULL);
    return last;
}

int main(int argc, char ** argv) {
#ifdef _WIN32
    //TODO: Implement named pipe Inter-Proccess-Communication(ICP)
    //Example: https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/ipc/transactions-on-named-pipes.md
#include <psapi.h>
    
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
		HWND hwnd = GetAllWindowsFromProcessID(processes[x]);
		//SendMessage(hwnd, WM_CLOSE, 0, 0);

		size_t len = strlen(argv[1]);
				
		COPYDATASTRUCT cds = {0};
		cds.dwData = 0;
		cds.cbData = len + 1;
		cds.lpData = argv[1];
		
		SendMessage(hwnd, WM_COPYDATA, (WPARAM) 0, (LPARAM) &cds);
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
		} else if(event.as.key == 'K') {
#ifdef _WIN32
		    //TODO: Weird behaviour, if the window is not closed
		    // It essentially blocks the thread, i think
		    char path[MAX_PATH];
		    
		    OPENFILENAME of = {0};
		    of.lStructSize = sizeof(OPENFILENAME);
		    of.hwndOwner = NULL;
		    of.lpstrFile = path;
		    of.lpstrFile[0] = '\0';
		    of.nMaxFile = sizeof(path );
		    of.lpstrFilter = "Image\0*.PNG;*.JPG\0All\0*.*\0";
		    of.nFilterIndex = 1;
		    of.lpstrFileTitle = NULL ;
		    of.nMaxFileTitle = 0 ;
		    of.lpstrInitialDir= NULL ;
		    of.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;
#ifdef _MSC_VER
#pragma comment(lib,"comdlg32.lib")
#endif //_MSVC_
		    if(GetOpenFileName(&of)) {
			maybe_load_file(path);
		    }
#endif //_WIN32
		} else if(event.as.key == 'V') {
#ifdef WIN32
		    if (! OpenClipboard(NULL)) {
			
		    }

		    // Get handle of clipboard object for ANSI text
		    HANDLE hData = GetClipboardData(CF_TEXT);
		    if (hData == NULL) {
			
		    }

		    // Lock the handle to get the actual text pointer
		    char * pszText = GlobalLock(hData);
		    if (pszText == NULL) {
			
		    }
		    
		    printf("%s\n", pszText); fflush(stdout);
		    
		    // Release the lock
		    GlobalUnlock( hData );
		    
		    // Release the clipboard
		    CloseClipboard();
		}
#endif //_WIN32
	    } else if(event.type == GUI_EVENT_FILEDROP) {
#ifdef _WIN32
		char path[MAX_PATH];
		HDROP h_drop = (HDROP) event.as.value;
		int count = DragQueryFile(h_drop, 0xffffffff, path, MAX_PATH);
		//TODO: handle multiple files somehow
		if(count > 1) {
		    for(int i=0;i<count;i++) {
			DragQueryFile(h_drop, i, path, MAX_PATH);
			printf("%s\n", path);
		    }
		    fflush(stdout);
		} else {
		    DragQueryFile(h_drop, 0, path, MAX_PATH);
		    maybe_load_file(path);
		}

		DragFinish(h_drop);
#endif //_WIN32
	    } else if(event.type == GUI_EVENT_MOUSEWHEEL) {
		z -= (float) event.as.amount;
		clampf(&z, 1.f, 20.f);
	    } else if(event.type == GUI_EVENT_DATARECEIVE) {
		const char *filepath = (char *) event.as.data.data;
		maybe_load_file(filepath);
		if( IsIconic(gui.win) ) {
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
