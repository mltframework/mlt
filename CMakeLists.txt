cmake_minimum_required(VERSION 3.14)
project(MLT)
set(MLT_VERSION 6.23.0)
find_package(PkgConfig REQUIRED)
include(GNUInstallDirs)
option(GPL "Enable GPLv2 modules" ON)
option(GPL3 "Enable GPLv3 modules" ON)
if(WIN32)
  option(NODEPLOY "Keep bin/ lib/ layout on Windows" ON)
endif()
if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(AMD64)")
    if(${ARCH} MATCHES "i686")
    else()
        set(X86_64 ON)
        if(NOT MSVC)
            add_definitions(-DUSE_MMX -DUSE_SSE -DUSE_SSE2 -DARCH_X86_64)
        endif()
    endif()
endif()
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-ffast-math)
elseif(MSVC)
    add_compile_options(/fp:fast)
endif()
find_package(Threads REQUIRED)
if (WIN32 AND NOT CMAKE_DL_LIBS)
    find_package(dlfcn-win32 REQUIRED)
    set(CMAKE_DL_LIBS dlfcn-win32::dl)
endif()
pkg_check_modules(FFTW IMPORTED_TARGET fftw3)
if(NOT FFTW_FOUND)
    pkg_check_modules(FFTW IMPORTED_TARGET fftw)
endif()
pkg_check_modules(xml IMPORTED_TARGET libxml-2.0)
add_subdirectory(src/framework)
add_subdirectory(src/melt)
add_subdirectory(src/mlt++)
add_subdirectory(src/swig)
#file(GLOB modules src/modules/*/)
set(modules
    src/modules/avformat
    src/modules/core
    src/modules/decklink
    src/modules/frei0r
    src/modules/feeds
    src/modules/gdk
    src/modules/jackrack
    src/modules/kdenlive
    src/modules/motion_est
    src/modules/normalize
    src/modules/oldfilm
    src/modules/opencv
    src/modules/opengl
    src/modules/plus
    src/modules/plusgpl
    src/modules/qt
    src/modules/resample
    src/modules/rubberband
    src/modules/rtaudio
    src/modules/sdl
    src/modules/sdl2
    src/modules/sox
    src/modules/vid.stab
    src/modules/vmfx
    src/modules/vorbis
    src/modules/xine
    src/modules/xml
    src/modules/lumas
)
foreach(module ${modules})
    add_subdirectory(${module})
endforeach()
install(DIRECTORY profiles DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt)
install(DIRECTORY presets DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt)
