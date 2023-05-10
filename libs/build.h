#ifndef BUILD_H_H
#define BUILD_H_H

#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#endif //_WIN32

#ifdef linux
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#endif //linux

#ifdef BUILD_IMPLEMENTATION
#define IO_IMPLEMENTATION
#endif //BUILD_IMPLEMENTATION

#include "./io.h"

#ifndef BUILD_DEF
#define BUILD_DEF static inline
#endif //BUILD_DEF

BUILD_DEF int run();
BUILD_DEF char *join(const char *delim);

BUILD_DEF bool gcc();
BUILD_DEF bool msvc();
BUILD_DEF bool is_windows();
BUILD_DEF bool is_linux();

#ifdef BUILD_IMPLEMENTATION

BUILD_DEF bool is_windows() {
#ifdef _WIN32
    return true;
#else
    return false;
#endif //_WIN32
}

BUILD_DEF bool is_linux() {
#ifdef linux
    return true;
#else
    return false;
#endif //linux
}

BUILD_DEF bool msvc() {
#ifdef _MSC_VER
    return true;
#else
    return false;
#endif //_MSC_VER
}

BUILD_DEF bool gcc() {
#ifdef __GNUC__
    return true;
#else
    return false;
#endif //__GNUC__
}

#define run(...) run_impl(1, __VA_ARGS__, NULL)

#define join(delim, ...) join2_impl(1, delim, __VA_ARGS__, NULL)

BUILD_DEF char *join_impl(const char *delim, va_list va) {

    size_t delim_len = strlen(delim);
    
#ifdef _MSC_VER
    va_list two = va;
#elif __GNUC__
    va_list two;
    va_copy(two, va);
#endif

    size_t len = 0;
    const char *cstr;
    for(;;) {
	cstr = va_arg(va, char *);
	if(!cstr) break;
	len += strlen(cstr);
	len += delim_len;
    }

    char *content = malloc(len + 1 * sizeof(char));
    if(!content) {
	fprintf(stderr, "ERROR: Can not allocate enough memory\n");
	exit(1);
    }

    len = 0;
    for(;;) {
	cstr = va_arg(two, char *);
	if(!cstr) break;
	size_t cstr_len = strlen(cstr);
	memcpy(content + len, cstr, cstr_len);
	len += cstr_len;
	memcpy(content + len, delim, delim_len);
	len += delim_len;
    }
    
    va_end(two);

    content[len - delim_len] = 0; // TODO: this leaks some memory
    return content;
}

BUILD_DEF char *join2_impl(int d, const char *delim, ...) {
    va_list va;
    va_start(va, delim);
    char *ret = join_impl(delim, va);
    va_end(va);
    return ret;
}

#define concat(...) concat2_impl(1, __VA_ARGS__, NULL)

BUILD_DEF const char ** concat_impl(va_list va) {
  
#ifdef _MSC_VER
  va_list va_copy = va;
#elif __GNUC__
  va_list va_copy;
  va_copy(va_copy, va);
#endif

  size_t len = 0;
  size_t cstr_len;
  const char *cstr;
  for(;;) {
    cstr = va_arg(va, char *);
    if(cstr) {
      cstr_len = strlen(cstr);    
      for(size_t i=0;i<cstr_len;i++) {
	if(cstr[i] == ' ') len++;
      }
    }
    len++;      
    if(!cstr) break;
  }

  const char **array = (const char **) malloc(len * sizeof(char**));
  if(!array) {
    fprintf(stderr, "ERROR: Can not allocate enough memory\n");
    exit(1);
  }

  len = 0;
  for(;;) {
    cstr = va_arg(va_copy, char *);
    if(cstr) {
      size_t last = 0;
      cstr_len = strlen(cstr);
      for(size_t i=0;i<cstr_len;i++) {
	if(cstr[i] == ' ') {
	  char *leaked_memory = malloc(sizeof(char) * ((i - last) + 1));
	  if(!leaked_memory) {
	    fprintf(stderr, "ERROR: Can not allocate enough memory\n");
	    exit(1);
	  }
	  memcpy(leaked_memory, cstr + last, i - last);
	  leaked_memory[i - last] = 0;
	  array[len++] = leaked_memory;
	  last = i + 1;
	}  
      }
      if(cstr_len) array[len++] = cstr + last;
    } else {
      array[len++] = cstr;
    }
    if(!cstr) break;
  }
  
  return array;
}

BUILD_DEF const char **concat2_impl(int d, ...) {
  va_list va;
  va_start(va, d);
  const char **ret = concat_impl(va);
  va_end(va);
  return NULL;
}

#ifdef linux
BUILD_DEF int run_impl(int d, ...) {
  va_list va;

  va_start(va, d);
  
  va_list va_copy;
  va_copy(va_copy, va);
  
  char *cmd_line = join_impl(" ", va);
  const char **commands = concat_impl(va_copy);
  va_end(va);

  printf("[RUNNING]: %s\n", cmd_line); fflush(stdout);

  pid_t cpid = fork();
  if(cpid < 0) {
    fprintf(stderr, "ERROR: Creating child process: '%s'\n", cmd_line);
    fprintf(stderr, "REASON: %s", strerror(errno));
    exit(1);
  }

  if(cpid == 0) {
    if(execvp(commands[0], (char * const*) commands) < 0) {
      fprintf(stderr, "ERROR: Executing execvp: '%s'\n", cmd_line);
      fprintf(stderr, "REASON: %s", strerror(errno));
      exit(1);
    }    
  }

  int exit_status = 0;
  for(;;) {
    int wstatus = 0;
    if(waitpid(cpid, &wstatus, 0) < 0) {
      fprintf(stderr, "ERROR: Can not wait on command: '%s'\n", cmd_line);
      fprintf(stderr, "REASON: %s", strerror(errno));
      exit(1);      
    }

    if(WIFEXITED(wstatus)) {
      exit_status = WEXITSTATUS(wstatus);
      break;
    }

    if(WIFSIGNALED(wstatus)) {
      fprintf(stderr, "ERROR: Command process was terminated: '%s'\n", cmd_line);
      fprintf(stderr, "By: %s", strsignal(WTERMSIG(wstatus)));
      exit(1);      
    }
  }
  
  return exit_status;
}
#endif //linux

#ifdef _WIN32
BUILD_DEF LPSTR GetLastErrorAsString(void)
{
    // https://stackoverflow.com/questions/1387064/how-to-get-the-error-message-from-the-error-code-returned-by-getlasterror

    DWORD errorMessageId = GetLastError();

    LPSTR messageBuffer = NULL;

    DWORD size =
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, // DWORD   dwFlags,
            NULL, // LPCVOID lpSource,
            errorMessageId, // DWORD   dwMessageId,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // DWORD   dwLanguageId,
            (LPSTR) &messageBuffer, // LPTSTR  lpBuffer,
            0, // DWORD   nSize,
            NULL // va_list *Arguments
        );

    return messageBuffer;
}

BUILD_DEF int run_impl(int d, ...) {
    va_list va;
    va_start(va, d);
    char *cmd_line = join_impl(" ", va);
    va_end(va);

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    printf("[RUNNING]: %s\n", cmd_line); fflush(stdout);
    
    BOOL bSuccess =
        CreateProcess(
            NULL,
            cmd_line,
            NULL,
            NULL,
            TRUE,
            0,
            NULL,
            NULL,
            &siStartInfo,
            &piProcInfo
	    );

    if (!bSuccess) {
	fprintf(stderr, "ERROR: Executing command: '%s'\n", cmd_line);
	fprintf(stderr, "REASON: %s", GetLastErrorAsString());
	exit(1);
    }    

    CloseHandle(piProcInfo.hThread);

    HANDLE pid = piProcInfo.hProcess;

    DWORD result = WaitForSingleObject(pid, INFINITE);

    if(result == WAIT_FAILED) {
	fprintf(stderr, "ERORR: Could not wait for process\n");
	fprintf(stderr, "REASON: %s", GetLastErrorAsString());
	exit(1);
    }

    DWORD exit_status;
    if (GetExitCodeProcess(pid, &exit_status) == 0) {
        fprintf(stderr, "ERROR: Could not get process exit code\n");
	fprintf(stderr, "REASON: %s", GetLastErrorAsString());
    }
    
    CloseHandle(pid);

    return (int) exit_status;
}
#endif //_WIN32

#endif //BUILD_IMPLEMENTATION

#endif //BUILD_H_H
