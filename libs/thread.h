#ifndef THREAD_H_H
#define THREAD_H_H

#ifdef _WIN32 ////////////////////////////////////////////
#include <windows.h>

#ifdef __GNUC__
#include <process.h>
#endif //__GNUC__

//#include <process.h>
typedef HANDLE Thread;
typedef HANDLE Mutex;
//TODO implement for gcc
#elif __GNUC__ ////////////////////////////////////////////
#include <pthread.h>
typedef pthread_t Thread;
typedef pthread_mutex_t Mutex;
#endif

#include <stdint.h> // for uintptr_t

int thread_create(Thread *id, void* (*function)(void *), void *arg);
void thread_join(Thread id);
void thread_sleep(int ms);

int mutex_create(Mutex* mutex);
void mutex_lock(Mutex mutex);
void mutex_release(Mutex mutex);

#ifdef THREAD_IMPLEMENTATION

#ifdef _WIN32 ////////////////////////////////////////////

int thread_create(Thread *id, void* (*function)(void *), void *arg) {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#endif //__GNUC__
  int ret = _beginthread(function, 0, arg);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__


  if(ret == -1) {
    return 0;
  }

  *id = (HANDLE) (uintptr_t) ret;

  return 1;
}

void thread_join(Thread id) {
  WaitForSingleObject(id, INFINITE);
  CloseHandle(id);
}

void thread_sleep(int ms) {
  Sleep(ms);
}

int mutex_create(Mutex* mutex) {
  *mutex = CreateMutexW(NULL, FALSE, NULL);
  return *mutex != NULL;
}

void mutex_lock(Mutex mutex) {
  WaitForSingleObject(mutex, INFINITE);
}

void mutex_release(Mutex mutex) {
  ReleaseMutex(mutex);
}

//TODO implement for gcc
#elif __GNUC__ ////////////////////////////////////////////

int thread_create(Thread *id, void* (*function)(void *), void *arg) {
  return pthread_create(id, NULL, function, arg) == 0;
}

void thread_join(Thread id) {
  pthread_join(id, NULL);
}

void thread_sleep(int ms) {
  //TOOD: proper sleep_time if ms is longer than a second
  struct timespec sleep_time;
  if(ms < 1000) {
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = ms * 1000000;    
  } else {
    sleep_time.tv_sec = ms / 1000;
    sleep_time.tv_nsec = 0;
  }
  if(nanosleep(&sleep_time, NULL) == -1) {
    return;
  }
}

#endif //__GNUC__

#endif //THREAD_IMPLEMENTATION


#endif //THREAD_H_H
