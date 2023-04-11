#ifndef THREAD_H_H
#define THREAD_H_H

#ifdef _WIN32 ////////////////////////////////////////////
#include <windows.h>
#include <process.h>
typedef HANDLE Thread;
typedef HANDLE Mutex;
//TODO implement for gcc
#elif __GNUC__ ////////////////////////////////////////////
#endif

#include <stdint.h> // for uintptr_t

int thread_create(Thread *id, void *function, void *arg);
void thread_join(Thread id);
void thread_sleep(int ms);

int mutex_create(Mutex* mutex);
void mutex_lock(Mutex mutex);
void mutex_release(Mutex mutex);

#ifdef THREAD_IMPLEMENTATION

#ifdef _WIN32 ////////////////////////////////////////////

int thread_create(Thread *id, void *function, void *arg) {
  int ret =
    //CreateThread(NULL, 0, function, arg, 0, NULL);
    _beginthread(function, 0, arg);
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

#endif //__GNUC__

#endif //THREAD_IMPLEMENTATION


#endif //THREAD_H_H
