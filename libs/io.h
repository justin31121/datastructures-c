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
  WIN32_FIND_DATAW file_data;
  HANDLE handle;
  bool stop;
#endif //_WIN32
  const char *name;
}Io_Dir;

typedef struct{
  bool is_dir;
  char abs_name[MAX_PATH];
  char *name;
}Io_File;

IO_DEF bool io_dir_open(Io_Dir *dir, char *dir_path);
IO_DEF bool io_dir_next(Io_Dir *dir, Io_File *file);
IO_DEF void io_dir_close(Io_Dir *dir);

#ifdef IO_IMPLEMENTATION

#ifdef _WIN32
IO_DEF bool io_dir_open(Io_Dir *dir, char *dir_path) {
  int num_wchars = MultiByteToWideChar(CP_UTF8, 0, dir_path, -1, NULL, 0);
  wchar_t *my_wstring = (wchar_t *)malloc((num_wchars+1) * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, dir_path, -1, my_wstring, num_wchars);
  my_wstring[num_wchars-1] = '*';
  my_wstring[num_wchars] = 0;

  // Use my_wstring as a const wchar_t *
  dir->handle = FindFirstFileExW(my_wstring, FindExInfoStandard, &dir->file_data, FindExSearchNameMatch, NULL, 0);
  if(dir->handle == INVALID_HANDLE_VALUE) {
    free(my_wstring);
    return false;
  }

  dir->name = dir_path;
  dir->stop = false;

  free(my_wstring);
  return true;
}

IO_DEF bool io_dir_next(Io_Dir *dir, Io_File *file) {

  if(dir->stop) {
    return false;
  }

  file->is_dir = (dir->file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;
  file->name = (char *) dir->file_data.cFileName;

  int len = strlen(dir->name);
  memcpy(file->abs_name, dir->name, len);
  int len2 = WideCharToMultiByte(CP_ACP, 0, dir->file_data.cFileName, -1, NULL, 0, NULL, NULL);
  WideCharToMultiByte(CP_ACP, 0, dir->file_data.cFileName, -1, file->abs_name + len, len2, NULL, NULL);
  file->abs_name[len + len2] = 0;

  if(FindNextFileW(dir->handle, &dir->file_data) == 0) {
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
