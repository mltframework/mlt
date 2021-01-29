cmake_minimum_required(VERSION 3.14)

project(MLT
  VERSION 6.25.0
  DESCRIPTION "Multimedia Framework"
  HOMEPAGE_URL "https://www.mltframework.org"
  LANGUAGES C CXX
)

option(GPL "Enable GPLv2 modules" ON)
option(GPL3 "Enable GPLv3 modules" ON)
option(BUILD_TESTING "Enable tests" OFF)

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

option(SWIG_PYTHON "Enable SWIG Python bindings" OFF)

if(WIN32)
  option(NODEPLOY "Keep bin/ lib/ layout on Windows" ON)
endif()

include(GNUInstallDirs)
if(BUILD_TESTING)
  include(CTest)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(AMD64)")
  if(${ARCH} MATCHES "i686")
  else()
    set(X86_64 ON)
    if(NOT MSVC)
      add_compile_definitions(USE_MMX USE_SSE USE_SSE2 ARCH_X86_64)
    endif()
  endif()
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-ffast-math)
elseif(MSVC)
  add_compile_options(/fp:fast)
endif()

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

if(WIN32)
  find_package(Iconv REQUIRED)
  if(NOT CMAKE_DL_LIBS)
    find_package(dlfcn-win32 REQUIRED)
    set(CMAKE_DL_LIBS dlfcn-win32::dl)
  endif()
endif()

pkg_check_modules(sdl2 IMPORTED_TARGET GLOBAL sdl2)

if(MOD_QT OR BUILD_TESTING)
  set(CMAKE_AUTOMOC ON)
  find_package(Qt5 COMPONENTS Core Gui Xml Svg Widgets Test)
endif()

if(MOD_QT OR MOD_PLUS)
  pkg_check_modules(FFTW IMPORTED_TARGET fftw3)
  if(NOT FFTW_FOUND)
    pkg_check_modules(FFTW IMPORTED_TARGET fftw)
  endif()
endif()

if(MOD_QT OR MOD_GDK)
  pkg_check_modules(libexif IMPORTED_TARGET libexif)
endif()

if(MOD_GDK OR MOD_GTK)
  pkg_check_modules(fontconfig IMPORTED_TARGET fontconfig)
endif()

if(MOD_XML OR MOD_JACKRACK)
  pkg_check_modules(xml IMPORTED_TARGET libxml-2.0)
endif()

if(MOD_AVFORMAT)
  pkg_check_modules(libavformat IMPORTED_TARGET libavformat)
  pkg_check_modules(libswscale IMPORTED_TARGET libswscale)
  pkg_check_modules(libavutil IMPORTED_TARGET libavutil)
  pkg_check_modules(libavcodec IMPORTED_TARGET libavcodec)
  pkg_check_modules(libavfilter IMPORTED_TARGET libavfilter)
  pkg_check_modules(libavdevice IMPORTED_TARGET libavdevice)
  pkg_check_modules(libswresample IMPORTED_TARGET libswresample)
endif()

if(MOD_FREI0R)
  pkg_check_modules(FREI0R frei0r)
endif()

if(MOD_GDK)
  pkg_check_modules(GdkPixbuf IMPORTED_TARGET gdk-pixbuf-2.0)
  pkg_check_modules(pango IMPORTED_TARGET pango)
  pkg_check_modules(pangoft2 IMPORTED_TARGET pangoft2)
endif()

if(MOD_GTK2)
  find_package(GTK2 COMPONENTS gtk)
endif()

if(MOD_JACKRACK)
  pkg_check_modules(jack IMPORTED_TARGET jack)
  pkg_check_modules(glib IMPORTED_TARGET glib-2.0)
  check_include_file(ladspa.h ladspa_h_FOUND)
endif()

if(MOD_OPENCV)
  find_package(OpenCV QUIET OPTIONAL_COMPONENTS tracking)
endif()

if(MOD_OPENGL)
  pkg_check_modules(movit IMPORTED_TARGET movit)
endif()

if(MOD_PLUS)
  pkg_check_modules(libebur128 IMPORTED_TARGET libebur128)
endif()

if(MOD_RESAMPLE)
  pkg_check_modules(samplerate IMPORTED_TARGET samplerate)
endif()

if(MOD_RTAUDIO)
  pkg_check_modules(rtaudio IMPORTED_TARGET rtaudio)
  if(UNIX AND NOT APPLE)
    pkg_check_modules(alsa IMPORTED_TARGET alsa)
    pkg_check_modules(libpulse-simple IMPORTED_TARGET libpulse-simple)
  endif()
endif()

if(MOD_RUBBERBAND)
  pkg_check_modules(rubberband IMPORTED_TARGET rubberband)
endif()

if(MOD_SDL1)
  pkg_check_modules(sdl IMPORTED_TARGET sdl)
  pkg_check_modules(sdl_image IMPORTED_TARGET sdl_image)
endif()

if(MOD_SOX)
  pkg_check_modules(sox IMPORTED_TARGET sox)
endif()

if(MOD_VIDSTAB)
  pkg_check_modules(vidstab IMPORTED_TARGET vidstab)
endif()

if(MOD_VORBIS)
  pkg_check_modules(vorbis IMPORTED_TARGET vorbis)
  pkg_check_modules(vorbisfile IMPORTED_TARGET vorbisfile)
endif()

find_package(SWIG)

if(SWIG_PYTHON)
  find_package(Python3 COMPONENTS Interpreter Development)
endif()

install(DIRECTORY presets profiles DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt)

if(UNIX AND NOT APPLE)
  install(FILES docs/melt.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()

add_subdirectory(src)
