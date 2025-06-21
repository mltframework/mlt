#ifndef SDL2_EXPORT_H
#define SDL2_EXPORT_H

#ifdef _WIN32
    #ifdef MLTSDL2_BUILD_DLL
        #define MLTSDL2_API __declspec(dllexport)
    #else
        #define MLTSDL2_API __declspec(dllimport)
    #endif
#else
    #define MLTSDL2_API
#endif

#endif // SDL2_EXPORT_H