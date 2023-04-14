#ifndef IO_H_H
#define IO_H_H

#ifdef _WIN32
#include <windows.h>
#include <stdbool.h>
#endif //_WIN32

#ifndef IO_DEF
#define IO_DEF static inline
#endif //IO_DEF

typedef struct{
#ifdef _WIN32
  WIN32_FIND_DATA file_data;
  HANDLE handle;
  bool stop;
#endif //_WIN32
  const char *name;
}Io_Dir;

typedef struct{
  const char *name;
  bool is_dir;
  char abs_name[MAX_PATH];
}Io_File;

IO_DEF bool io_dir_open(Io_Dir *dir, const char *dir_path);
IO_DEF bool io_dir_next(Io_Dir *dir, Io_File *file);
IO_DEF void io_dir_close(Io_Dir *dir);

#ifdef IO_IMPLEMENTATION

#ifdef _WIN32
IO_DEF bool io_dir_open(Io_Dir *dir, const char *dir_path) {
  char buffer[MAX_PATH+1];
  int len = strlen(dir_path);
  memcpy(buffer, dir_path, len);
  buffer[len] = '*';
  buffer[len+1] = 0;
  
  dir->handle = FindFirstFile(buffer, &dir->file_data);
  if(dir->handle == INVALID_HANDLE_VALUE) {
    return false;
  }

  dir->name = dir_path;
  dir->stop = false;
  
  return true;
}

IO_DEF bool io_dir_next(Io_Dir *dir, Io_File *file) {

  if(dir->stop) {
    return false;
  }

  file->is_dir = (dir->file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;
  file->name = dir->file_data.cFileName;

#ifndef IO_FAST
  int len = strlen(dir->name);
  memcpy(file->abs_name, dir->name, len);
  int len2 = strlen(file->name);
  memcpy(file->abs_name + len, file->name, len2);
  file->abs_name[len + len2] = 0;
#endif //IO_FAST

  if(FindNextFile(dir->handle, &dir->file_data) == 0) {
    dir->stop = true;
  }

  return true;
}

IO_DEF void io_dir_close(Io_Dir *dir) {
  FindClose(dir->handle);
}
#endif //_WIN32

#endif //IO_IMPLEMENTATION

#endif //_IO_H_
