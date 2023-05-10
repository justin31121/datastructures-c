#ifndef IO_H_H
#define IO_H_H

#include <stdbool.h>

#ifdef _WIN32
# include <windows.h>
# include <stdio.h>
#endif //_WIN32

#ifdef linux
# include <limits.h>
# include <dirent.h>
# include <sys/stat.h>
#endif //linux

#ifndef IO_DEF
#define IO_DEF static inline
#endif //IO_DEF

typedef struct{
#ifdef _WIN32
  WIN32_FIND_DATAW file_data;
  HANDLE handle;
  bool stop;
#endif //_WIN32

#ifdef linux
  struct dirent *ent;
  DIR *handle;
#endif //linux

  const char *name;
}Io_Dir;

typedef struct{
  bool is_dir;
#ifdef _WIN32
  char abs_name[MAX_PATH];
#elif linux
  char abs_name[PATH_MAX];
#endif //_WIN32
  char *name;
}Io_File;

IO_DEF bool io_dir_open(Io_Dir *dir, const char *dir_path);
IO_DEF bool io_dir_next(Io_Dir *dir, Io_File *file);
IO_DEF void io_dir_close(Io_Dir *dir);

IO_DEF bool io_exists(const char *file_path, bool *is_file);
IO_DEF bool io_delete(const char *file_path);

#ifdef IO_IMPLEMENTATION

#ifdef linux

IO_DEF bool io_dir_open(Io_Dir *dir, const char *dir_path) {

  dir->handle = opendir(dir_path);
  if(dir->handle == NULL) {
    return false;
  }  

  dir->name = dir_path;  
  return true;
}

IO_DEF bool io_dir_next(Io_Dir *dir, Io_File *file) {

  dir->ent = readdir(dir->handle);
  if(dir->ent == NULL) {
    return false;
  }

  file->is_dir = (dir->ent->d_type == DT_DIR) != 0;
  file->name = dir->ent->d_name + 1;
  int len = strlen(dir->name);
  memcpy(file->abs_name, dir->name, len);
  int len2 = strlen(dir->ent->d_name);
  memcpy(file->abs_name + len, dir->ent->d_name, len2);
  file->abs_name[len + len2] = 0;
  
  return true;
}

IO_DEF void io_dir_close(Io_Dir *dir) {
  closedir(dir->handle);
}

IO_DEF bool io_exists(const char *file_path, bool *is_file) {
  struct stat file_info;
  if(stat(file_path, &file_info) == 0) {        
    bool file = S_ISREG(file_info.st_mode) != 0;
    if(is_file) *is_file = file;
    
    return file || S_ISDIR(file_info.st_mode) != 0;
  }

  return false;
}

#endif //linux

#ifdef _WIN32
IO_DEF bool io_dir_open(Io_Dir *dir, const char *dir_path) {
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

  bool is_dir = (dir->file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;
  if(!is_dir) {
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

IO_DEF bool io_exists(const char *file_path, bool *is_file) {
    DWORD attribs = GetFileAttributes(file_path);
    if(is_file) *is_file = !(attribs & FILE_ATTRIBUTE_DIRECTORY);
    return attribs != INVALID_FILE_ATTRIBUTES;
}
#endif //_WIN32

IO_DEF bool io_delete(const char *file_path) {
  return remove(file_path) == 0;
}

#endif //IO_IMPLEMENTATION

#endif //_IO_H_
