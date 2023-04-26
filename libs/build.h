#ifndef BUILD_H_H
#define BUILD_H_H

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>

#ifndef BUILD_DEF
#define BUILD_DEF static inline
#endif //BUILD_DEF

BUILD_DEF int run();
BUILD_DEF void rebuild_urself(int argc, char **argv);
BUILD_DEF char *join(const char *delim);

#ifdef BUILD_IMPLEMENTATION

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

#define run(...) run_impl(1, __VA_ARGS__, NULL)

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

    printf("[RUNNING]: %s\n", cmd_line);
    
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

#ifndef REBUILD_URSELF
#  if _WIN32
#    if defined(__GNUC__)
#       define REBUILD_URSELF(binary_path, source_path) run("gcc", "-o", binary_path, source_path)
#    elif defined(__clang__)
#       define REBUILD_URSELF(binary_path, source_path) run("clang", "-o", binary_path, source_path)
#    elif defined(_MSC_VER)
#       define REBUILD_URSELF(binary_path, source_path) run("cl.exe", source_path)
#    endif
#  else
#    define REBUILD_URSELF(binary_path, source_path) CMD("cc", "-o", binary_path, source_path)
#  endif
#endif

BUILD_DEF HANDLE fd_open_for_read(const char *path) {
    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    HANDLE result = CreateFile(
                    path,
                    GENERIC_READ,
                    0,
                    &saAttr,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_READONLY,
                    NULL);

    if (result == INVALID_HANDLE_VALUE) {
	fprintf(stderr, "ERROR: Could not open file: '%s'\n", path);
	exit(1);
    }

    return result;
}

BUILD_DEF int is_path1_modified_after_path2(const char *path1, const char *path2)
{
    FILETIME path1_time, path2_time;

    HANDLE path1_fd = fd_open_for_read(path1);
    if (!GetFileTime(path1_fd, NULL, NULL, &path1_time)) {
        fprintf(stderr, "ERROR: Could not get time of '%s'\n", path1);
	fprintf(stderr, "REASON: %s", path1, GetLastErrorAsString());
	exit(1);
    }
    CloseHandle(path1_fd);

    HANDLE path2_fd = fd_open_for_read(path2);
    if (!GetFileTime(path2_fd, NULL, NULL, &path2_time)) {
	fprintf(stderr, "ERROR: Could not get time of '%s'\n", path2);
	fprintf(stderr, "REASON: %s", path1, GetLastErrorAsString());
	exit(1);
    }
    CloseHandle(path2_fd);

    return CompareFileTime(&path1_time, &path2_time) == 1;
}

#define rebuild_urself(argc, argv) do {					\
	const char *source = __FILE__;					\
	const char *binary = join2_impl(1, ".", argv[0], "exe", NULL);	\
	const char *binary_temp = join2_impl(1, ".", argv[0], "exe", "temp"); \
									\
	if(is_path1_modified_after_path2(source, binary)) {		\
	    int ret = REBUILD_URSELF(binary, source);			\
	    if(ret != 0) {						\
		exit(ret);						\
	    }								\
	    size_t len = 0;						\
	    for(int i=0;i<argc;i++) {					\
		len += strlen(argv[i]);					\
		if(i != argc-1) len += 1;				\
	    }								\
									\
	    char *content = malloc(len + 1);				\
	    if(!content) {						\
		fprintf(stderr, "ERROR: Can not allocate enough memory\n"); \
		exit(1);						\
	    }								\
									\
	    len = 0;							\
	    for(int i=0;i<argc;i++) {					\
		size_t arg_len = strlen(argv[i]);			\
		memcpy(content + len, argv[i], arg_len);		\
		len += arg_len;						\
		if(i != argc-1) content[len++] = ' ';			\
	    }								\
	    content[len] = 0;						\
									\
	    printf("now i just have to execute: '%s'\n", content);	\
	    exit(0);							\
	}								\
    }while(0)
#endif //BUILD_IMPLEMENTATION

#endif //BUILD_H_H
