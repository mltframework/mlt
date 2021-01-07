cmake_minimum_required(VERSION 3.14)

project(MLT)

set(MLT_VERSION 6.25.0)

option(GPL "Enable GPLv2 modules" ON)
option(GPL3 "Enable GPLv3 modules" ON)

option(MOD_AVFORMAT "Enable avformat module" ON)
option(MOD_DECKLINK "Enable decklink module" ON)
option(MOD_FEEDS "Enable feeds module" ON)
option(MOD_FREI0R "Enable frei0r module" ON)
option(MOD_GDK "Enable gdk module" ON)
option(MOD_GTK2 "Enable gtk2 module" ON)
option(MOD_JACKRACK "Enable jackrack module" ON)
option(MOD_KDENLIVE "Enable kdenlive module" ON)
option(MOD_LUMAS "Enable lumas module" ON)
option(MOD_MOTION_EST "Enable motion estimation module" ON)
option(MOD_NORMALIZE "Enable normalize module" ON)
option(MOD_OLDFILM "Enable oldfilm module" ON)
option(MOD_OPENCV "Enable OpenCV module" ON)
option(MOD_OPENGL "Enable OpenGL module" ON)
option(MOD_PLUS "Enable plus module" ON)
option(MOD_PLUSGPL "Enable plus GPL module" ON)
option(MOD_QT "Enable Qt module" ON)
option(MOD_RESAMPLE "Enable resample module" ON)
option(MOD_RTAUDIO "Enable RtAudio module" ON)
option(MOD_RUBBERBAND "Enable rubberband module" ON)
option(MOD_SDL1 "Enable SDL1 module" ON)
option(MOD_SDL2 "Enable SDL2 module" ON)
option(MOD_SOX "Enable SoX module" ON)
option(MOD_VIDSTAB "Enable vid.stab module" ON)
option(MOD_VMFX "Enable vmfx module" ON)
option(MOD_VORBIS "Enable Vorbis module" ON)
option(MOD_XINE "Enable xine module" ON)
option(MOD_XML "Enable XML module" ON)

if(WIN32)
  option(NODEPLOY "Keep bin/ lib/ layout on Windows" ON)
endif()

find_package(PkgConfig REQUIRED)
include(GNUInstallDirs)

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
