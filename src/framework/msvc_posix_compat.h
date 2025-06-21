#ifndef MSVC_COMPAT_H
#define MSVC_COMPAT_H

#ifdef _MSC_VER // 只在 MSVC 编译器下生效

// ------------------------------------------------------------------
// 关键部分：建立一个干净且完整的 Windows/Winsock 环境
// ------------------------------------------------------------------

// 1. 定义 WIN32_LEAN_AND_MEAN 来减少不必要的包含，避免冲突
//    但是，对于 COM，我们可能需要完整的 windows.h，所以可以暂时注释掉它来测试
// #ifndef WIN32_LEAN_AND_MEAN
// #define WIN32_LEAN_AND_MEAN
// #endif

// 2. 确保 Winsock2 在最前面，解决 sockaddr 重定义问题
#include <winsock2.h>
#include <ws2tcpip.h>

// 3. 包含完整的 Windows 头文件，它会定义 'interface' 等 COM 关键字
//    这一步是解决当前问题的关键
#include <windows.h>

// 4. 包含其他 COM 核心头文件，确保万无一失
#include <objbase.h>


// ------------------------------------------------------------------
// POSIX 兼容性定义
// ------------------------------------------------------------------
#include <BaseTsd.h>
#include <io.h>
#include <string.h>

typedef SSIZE_T ssize_t;
#define STDIN_FILENO  _fileno(stdin)
#define STDOUT_FILENO _fileno(stdout)
#define STDERR_FILENO _fileno(stderr)
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp

#endif // _MSC_VER

#endif // MSVC_COMPAT_H