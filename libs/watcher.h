#ifndef WATCHER_H_H
#define WATCHER_H_H

#include <stdbool.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif //_WIN32

#ifdef linux
#include <unistd.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <pthread.h>
#endif //linux

#ifdef linux
#define WATCHER_EVENT_SIZE  ( sizeof (struct inotify_event) )
#define WATCHER_EVENT_BUF_LEN ( 1024 * ( WATCHER_EVENT_SIZE + 16 ) )
#endif //linux

typedef enum {
	WATCHER_EVENT_UNKNOWN = 0,
	WATCHER_EVENT_CREATE,
	WATCHER_EVENT_DELETE,
	WATCHER_EVENT_MODIFY,
	WATCHER_EVENT_MOVE,
}Watcher_Event;

typedef struct{
	void (*onEvent)(Watcher_Event event, const char *file_name);
#ifdef _WIN32
	HANDLE dh;
	HANDLE id;
#endif //_WIN32  
#ifdef linux
	int fd;
	int wd;
	pthread_t id;
#endif //linux

	bool running;
}Watcher;

bool watcher_init(Watcher *watcher, const char *file_path, void (*onEvent)(Watcher_Event, const char *));
bool watcher_start(Watcher *watcher);
void *watcher_watch_function(void *arg);
void watcher_free(Watcher *watcher);

#ifdef _WIN32
Watcher_Event watcher_get_event(FILE_NOTIFY_INFORMATION *notify);
#endif //_WIN32
#ifdef linux
Watcher_Event watcher_get_event(const struct inotify_event *event);
#endif //linux
const char *watcher_event_name(Watcher_Event event);

#ifdef WATCHER_IMPLEMENTATION

bool watcher_init(Watcher *watcher, const char *file_path, void (*onEvent)(Watcher_Event, const char *)) {
	if(!watcher) {
		return false;
	}

	*watcher = (Watcher) {0};
	watcher-> onEvent = onEvent;

#ifdef _WIN32
	watcher->dh = CreateFile("./rsc/",
			     // access (read/write) mode
		GENERIC_READ,
			     // shar mode
		FILE_SHARE_READ | FILE_SHARE_WRITE |FILE_SHARE_DELETE,
			     // security descriptor
		NULL,
			     // how to create
		OPEN_EXISTING,
			     // file attributes
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED ,
			     // file with attributes to copy
		NULL );

	if(watcher->dh == INVALID_HANDLE_VALUE) {
		printf("%d\n", GetLastError());
		return false;
	}
#endif //_WIN32

#ifdef linux
	watcher->fd = inotify_init1(IN_NONBLOCK);
	if(watcher->fd < 0) {
		return false;
	}  
	watcher->wd = inotify_add_watch(watcher->fd, file_path, IN_CREATE | IN_DELETE | IN_CLOSE_WRITE | IN_MOVE);
#endif //linux

	return true;
}

bool watcher_start(Watcher *watcher) {
	if(!watcher) {
		return false;
	}

#ifdef _WIN32
	HANDLE ret = (HANDLE) _beginthread(watcher_watch_function, 0, watcher);
	if(ret < 0) {
		printf("hererer\n");
		return false;
	}
	watcher->id = ret;
#endif //_WIN32


#ifdef linux
	if(pthread_create(&watcher->id, NULL, watcher_watch_function, watcher)) {
		return false;
	}
#endif //linux    
	watcher->running = true;

	return true;
}

#ifdef _WIN32
typedef struct{
	BYTE *buffer;
	bool *skipNext;
	Watcher *watcher;
}CallBackStruct;

void ProcessFileNotifyInformation(FILE_NOTIFY_INFORMATION *notify, bool *skipNext, Watcher *watcher)
{  
	char filename[MAX_PATH];
	if(!notify->FileNameLength) return;

    //get filename
	filename[0] = notify->FileName[0];
	int j = 1;
	for(size_t i=2;i<notify->FileNameLength;i+=2) {
		filename[j++] = (char) ((char *) notify->FileName)[i];
	}
	filename[j] = 0;

	if(filename[0] == '.' || filename[1] == '#') return;
	if(*skipNext) {
		*skipNext = false;
		if(notify->Action == FILE_ACTION_MODIFIED) return;	    
	}

	Watcher_Event watcher_event = watcher_get_event(notify);
	if(watcher_event == WATCHER_EVENT_CREATE ||
		watcher_event == WATCHER_EVENT_MODIFY) {
		*skipNext = true;
}

watcher->onEvent(watcher_event, filename);

if(notify->NextEntryOffset != 0) {
	notify = (FILE_NOTIFY_INFORMATION *)(((LPBYTE) notify) + notify->NextEntryOffset);
	ProcessFileNotifyInformation(notify, skipNext, watcher);
}
}

void CALLBACK ReadDirectoryChangesCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	(void) dwErrorCode;
	(void) dwNumberOfBytesTransfered;

	printf("in the callback\n");

	CallBackStruct *callBackStruct = (CallBackStruct *) lpOverlapped->Pointer;
	BYTE *buffer = callBackStruct->buffer;
	bool *skipNext = callBackStruct->skipNext;
	Watcher *watcher = callBackStruct->watcher;
	FILE_NOTIFY_INFORMATION *pNotifyInfo = (FILE_NOTIFY_INFORMATION *) (buffer);
	ProcessFileNotifyInformation(pNotifyInfo, skipNext, watcher);
}

#endif //_WIN32

void *watcher_watch_function(void *arg) {
	Watcher *watcher = (Watcher *) arg;

#ifdef _WIN32
	BYTE buffer[4096];
	bool skipNext = false;

	DWORD bytes_returned;
	for(;watcher->running;){
		int ret = ReadDirectoryChangesW(watcher->dh,
					//read results buffer
			&buffer,
					//results buffer size
			sizeof(buffer),
					// maybe watch subdirectories ?
			FALSE,
					// filter conditions
			FILE_NOTIFY_CHANGE_SECURITY |
			FILE_NOTIFY_CHANGE_CREATION |
			FILE_NOTIFY_CHANGE_LAST_ACCESS |
			FILE_NOTIFY_CHANGE_LAST_WRITE |
			FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_ATTRIBUTES |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_FILE_NAME,
					//bytes returned
			&bytes_returned,
					//overlapped buffers
			NULL,
					//completion routine
			NULL);
		if(!ret) break;
		FILE_NOTIFY_INFORMATION *notify = (FILE_NOTIFY_INFORMATION *) (buffer);
		ProcessFileNotifyInformation(notify, &skipNext, watcher);
	}
#endif //_WIN32

#ifdef linux
	const struct inotify_event *event;
	char buf[WATCHER_EVENT_BUF_LEN];
	bool skip = false;
	while(watcher->running) {
		bool attemptToRename = false;
		fd_set descriptors = {0};
		struct timeval time_to_wait;
		time_to_wait.tv_sec = 0;
		time_to_wait.tv_usec = 500;

		FD_SET(watcher->fd, &descriptors);

		int ret = select(watcher->fd + 1, &descriptors,
			NULL, NULL, &time_to_wait);
		if(ret < 0) {
			watcher->running = false;
			fprintf(stderr, "watcher.h: WARNING: select() failed\n");
			break;
		} else if(!ret) {
			continue;
		}

		ssize_t len = read(watcher->fd, buf, WATCHER_EVENT_BUF_LEN);
		if(len < 0) {
			watcher->running = false;
			fprintf(stderr, "watcher.h: WARNING: read() failed\n");
			break;
		}

		for(char *ptr = buf; ptr < buf + len;
			ptr += sizeof(struct inotify_event) + event->len) {
			event = (const struct inotify_event *) ptr;
		if(!watcher->running) {
			break;
		}
		if(!event->len) {
			continue;
		}
		if(event->name[0] == '.' || event->name[0] == '#') {
			continue;
		}

		Watcher_Event watcher_event = watcher_get_event(event);
		if(watcher_event == WATCHER_EVENT_UNKNOWN) {
			continue;
		}

		bool skipNext = false;
		if(watcher_event == WATCHER_EVENT_CREATE) {
			skipNext = true;
		}

		if(watcher_event == WATCHER_EVENT_MOVE && attemptToRename) {
			watcher_event = WATCHER_EVENT_CREATE;
			attemptToRename = false;
		}

		if(!skip && watcher->onEvent != NULL) {
			Watcher_Event user_event = watcher_event;
			if(user_event == WATCHER_EVENT_MOVE) user_event = WATCHER_EVENT_DELETE;
			watcher->onEvent(user_event, event->name);
		}

		if(watcher_event == WATCHER_EVENT_MOVE) {
			attemptToRename = true;
		}

		skip = skipNext;
	}
}
#endif //linux

return NULL;
}

void watcher_free(Watcher *watcher) {
	if(!watcher) {
		return;
	}

	watcher->running = false;

#ifdef _WIN32
	WaitForSingleObject(watcher->id, INFINITE);

	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	SetEvent(hEvent);
	CloseHandle(watcher->dh);
#endif //_WIN32

#ifdef linux
	pthread_join(watcher->id, NULL);

	inotify_rm_watch(watcher->fd, watcher->wd);  
	close(watcher->fd);
#endif //linux
}

#ifdef _WIN32
Watcher_Event watcher_get_event(FILE_NOTIFY_INFORMATION *notify) {
	if(!notify) {
		return WATCHER_EVENT_UNKNOWN;
	}

	switch(notify->Action) {
		case FILE_ACTION_MODIFIED: 
		return WATCHER_EVENT_MODIFY;
		case FILE_ACTION_ADDED:
		return WATCHER_EVENT_CREATE;
		case FILE_ACTION_REMOVED:
		return WATCHER_EVENT_DELETE;
		case FILE_ACTION_RENAMED_OLD_NAME: 
		return WATCHER_EVENT_DELETE;
		case FILE_ACTION_RENAMED_NEW_NAME:
		return WATCHER_EVENT_CREATE;
		default:
		return WATCHER_EVENT_UNKNOWN;
	}
}
#endif //_WIN32

#ifdef linux
Watcher_Event watcher_get_event(const struct inotify_event *event) {
	if(!event) {
		return WATCHER_EVENT_UNKNOWN;
	}

	if(event->mask & IN_CREATE) {
		return WATCHER_EVENT_CREATE;
	} else if(event->mask & IN_DELETE) {
		return WATCHER_EVENT_DELETE;
	} else if(event->mask & IN_MOVE) {
		return WATCHER_EVENT_MOVE;
	} else if(event->mask & IN_CLOSE_WRITE) {
		return WATCHER_EVENT_MODIFY;
	} else {
		return WATCHER_EVENT_UNKNOWN;
	}

}
#endif //linux

const char *watcher_event_name(Watcher_Event event) {
	switch(event) {
		case WATCHER_EVENT_CREATE:
		return "created";
		case WATCHER_EVENT_DELETE:
		return "deleted";
		case WATCHER_EVENT_MODIFY:
		return "modified";
		case WATCHER_EVENT_MOVE:
		return "deleted";
		case WATCHER_EVENT_UNKNOWN:
		default:    
		return "unknown";
	}
}

#endif //WATCHER_IMPLEMENTATION

#endif //WATCHER_H_H
