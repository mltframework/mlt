#ifndef MLT_API_H
#define MLT_API_H

#ifdef SWIG
    #define MLT_API
#else
    #if defined(_WIN32) && defined(_MSC_VER)
        #ifdef MLT_EXPORTS 
            #define MLT_API __declspec(dllexport)
        #else 
            #define MLT_API __declspec(dllimport)
        #endif
    #elif defined(__GNUC__) || defined(__clang__)
        #ifdef MLT_EXPORTS
            #define MLT_API __attribute__((visibility("default")))
        #else
            #define MLT_API 
        #endif
    #else
        #define MLT_API
    #endif

#endif // SWIG

#endif // MLT_API_H