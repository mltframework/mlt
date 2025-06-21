//
// Created by sammiller on 2025/6/21.
//

#ifndef MLT_API_H
#define MLT_API_H
#if defined(_WIN32) && defined(_MSC_VER)
  #ifdef MLT_EXPORTS
    #define MLT_API __declspec(dllexport)
  #else
    #define MLT_API __declspec(dllimport)
  #endif
#else
  #define MLT_API
#endif

#endif //MLT_API_H
