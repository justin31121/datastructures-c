#ifndef WATCHER_H_H
#define WATCHER_H_H

#include <unistd.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>

#define WATCHER_EVENT_SIZE  ( sizeof (struct inotify_event) )
#define WATCHER_EVENT_BUF_LEN ( 1024 * ( WATCHER_EVENT_SIZE + 16 ) )

typedef enum {
	      WATCHER_EVENT_UNKNOWN = 0,
	      WATCHER_EVENT_CREATE,
	      WATCHER_EVENT_DELETE,
	      WATCHER_EVENT_MODIFY,
	      WATCHER_EVENT_MOVE,
}Watcher_Event;

typedef struct{
  void (*onEvent)(Watcher_Event event, const char *file_name);
  int fd;
  int wd;
  pthread_t id;
  bool running;
}Watcher;

bool watcher_init(Watcher *watcher, const char *file_path, void (*onEvent)(Watcher_Event, const char *));
bool watcher_start(Watcher *watcher);
void *watcher_watch_function(void *arg);
void watcher_free(Watcher *watcher);

Watcher_Event watcher_get_event(const struct inotify_event *event);
const char *watcher_event_name(Watcher_Event event);

#ifdef WATCHER_IMPLEMENTATION

bool watcher_init(Watcher *watcher, const char *file_path, void (*onEvent)(Watcher_Event, const char *)) {
  if(!watcher) {
    return false;
  }

  *watcher = (Watcher) {0};
  watcher-> onEvent = onEvent;
  
  watcher->fd = inotify_init1(IN_NONBLOCK);
  if(watcher->fd < 0) {
    return false;
  }
  
  watcher->wd = inotify_add_watch(watcher->fd, file_path, IN_CREATE | IN_DELETE | IN_CLOSE_WRITE | IN_MOVE);

  return true;
}

bool watcher_start(Watcher *watcher) {
  if(!watcher) {
    return false;
  }

  if(pthread_create(&watcher->id, NULL, watcher_watch_function, watcher)) {
    return false;
  }
  watcher->running = true;
  
  return true;
}

void *watcher_watch_function(void *arg) {
  Watcher *watcher = (Watcher *) arg;

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
  
  return NULL;
}

void watcher_free(Watcher *watcher) {
  if(!watcher) {
    return;
  }

  watcher->running = false;
  pthread_join(watcher->id, NULL);
  
  inotify_rm_watch(watcher->fd, watcher->wd);  
  close(watcher->fd);
}

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
