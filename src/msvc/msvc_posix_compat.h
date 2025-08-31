#ifndef MSVC_COMPAT_H
#define MSVC_COMPAT_H
#ifdef _MSC_VER 
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <objbase.h>
#include <BaseTsd.h>
#include <io.h>
#include <string.h>
#include <malloc.h>
typedef SSIZE_T ssize_t;
#define STDIN_FILENO  _fileno(stdin)
#define STDOUT_FILENO _fileno(stdout)
#define STDERR_FILENO _fileno(stderr)
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#define alloca _alloca 
#define tzname      _tzname
#define isatty       _isatty
static inline int close(int fd)
{
    return _close(fd);
}
static inline int write(int fd, const void *buffer, unsigned int count)
{
    return _write(fd, buffer, count);
}
#endif // _MSC_VER
#endif // MSVC_COMPAT_H