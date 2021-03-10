cmake_minimum_required(VERSION 3.14)

project(MLT
  VERSION 7.0.0
  DESCRIPTION "Multimedia Framework"
  HOMEPAGE_URL "https://www.mltframework.org"
  LANGUAGES C CXX
)

option(GPL "Enable GPLv2 modules" ON)
option(GPL3 "Enable GPLv3 modules" ON)
option(BUILD_TESTING "Enable tests" OFF)
option(BUILD_DOCS "Enable Doxygen documentation" OFF)

option(MOD_AVFORMAT "Enable avformat module" ON)
option(MOD_DECKLINK "Enable decklink module" ON)
option(MOD_FREI0R "Enable frei0r module" ON)
option(MOD_GDK "Enable gdk module" ON)
option(MOD_JACKRACK "Enable jackrack module" ON)
option(MOD_KDENLIVE "Enable kdenlive module" ON)
option(MOD_MOTION_EST "Enable motion estimation module" ON)
option(MOD_NDI "Enable NDI module" OFF)
option(MOD_NORMALIZE "Enable normalize module" ON)
option(MOD_OLDFILM "Enable oldfilm module" ON)
option(MOD_OPENCV "Enable OpenCV module" OFF)
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

option(SWIG_CSHARP "Enable SWIG CSharp bindings" OFF)
option(SWIG_JAVA "Enable SWIG Java bindings" OFF)
option(SWIG_LUA "Enable SWIG Lua bindings" OFF)
option(SWIG_NODEJS "Enable SWIG Node.js bindings" OFF)
option(SWIG_PERL "Enable SWIG Perl bindings" OFF)
option(SWIG_PHP "Enable SWIG PHP bindings" OFF)
option(SWIG_PYTHON "Enable SWIG Python bindings" OFF)
option(SWIG_RUBY "Enable SWIG Ruby bindings" OFF)
option(SWIG_TCL "Enable SWIG Tcl bindings" OFF)

if(WIN32)
  option(WINDOWS_DEPLOY "Install exes/libs directly to prefix (no subdir /bin)" ON)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

if(WINDOWS_DEPLOY)
  set(CMAKE_INSTALL_BINDIR .)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib")
set(MLT_MODULE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib/mlt")
set(MLT_DATA_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/share/mlt")

if(WIN32) # symlinks require admin rights on Windows
  file(COPY "${CMAKE_SOURCE_DIR}/src/modules" DESTINATION "${CMAKE_BINARY_DIR}/out/share" FILES_MATCHING REGEX yml|txt)
  file(RENAME "${CMAKE_BINARY_DIR}/out/share/modules" "${MLT_DATA_OUTPUT_DIRECTORY}")
  file(COPY "${CMAKE_SOURCE_DIR}/presets" DESTINATION "${MLT_DATA_OUTPUT_DIRECTORY}")
  file(COPY "${CMAKE_SOURCE_DIR}/profiles" DESTINATION "${MLT_DATA_OUTPUT_DIRECTORY}")
else()
  file(MAKE_DIRECTORY "${MLT_DATA_OUTPUT_DIRECTORY}")
  file(GLOB MOD_SUBDIRS "${CMAKE_SOURCE_DIR}/src/modules/*")
  foreach(MOD_SUBDIR ${MOD_SUBDIRS})
    file(RELATIVE_PATH MOD_NAME "${CMAKE_SOURCE_DIR}/src/modules" ${MOD_SUBDIR})
    file(CREATE_LINK "${CMAKE_SOURCE_DIR}/src/modules/${MOD_NAME}" "${MLT_DATA_OUTPUT_DIRECTORY}/${MOD_NAME}" SYMBOLIC)
  endforeach()
  file(CREATE_LINK "${CMAKE_SOURCE_DIR}/presets" "${MLT_DATA_OUTPUT_DIRECTORY}/presets" SYMBOLIC)
  file(CREATE_LINK "${CMAKE_SOURCE_DIR}/profiles" "${MLT_DATA_OUTPUT_DIRECTORY}/profiles" SYMBOLIC)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND MLT_COMPILE_OPTIONS "")

# MSVC cl doesn't support GNU inline assembly (but MSVC-compatible clang-cl does)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "i686|x86|x86_64|AMD64")
    set(CPU_MMX ON)
    set(CPU_SSE ON)
    set(CPU_SSE2 ON)
    if(NOT MSVC) # also NOT clang-cl
      list(APPEND MLT_COMPILE_OPTIONS "-mmmx;-msse;-msse2")
    endif()
  endif()
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "i686" OR (WIN32 AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86"))
    set(CPU_X86_32 ON)
  endif()
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(CPU_X86_64 ON)
  endif()
endif()

if(MSVC)
  list(APPEND MLT_COMPILE_OPTIONS "$<$<CONFIG:Release>:/fp:fast>")
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  list(APPEND MLT_COMPILE_OPTIONS "$<$<CONFIG:Release>:-ffast-math>")
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

if(BUILD_TESTING)
  find_package(Qt5 REQUIRED COMPONENTS Core Test)
  find_package(Kwalify REQUIRED)
  enable_testing()
endif()

if(MOD_QT)
  find_package(Qt5 COMPONENTS Core Gui Xml Svg Widgets)
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

if(MOD_JACKRACK)
  pkg_check_modules(jack IMPORTED_TARGET jack)
  pkg_check_modules(glib IMPORTED_TARGET glib-2.0)
  check_include_file(ladspa.h ladspa_h_FOUND)
endif()

if(MOD_NDI)
  find_package(NDI REQUIRED)
endif()

if(MOD_OPENCV)
  find_package(OpenCV QUIET OPTIONAL_COMPONENTS tracking)
endif()

if(MOD_OPENGL)
  pkg_check_modules(movit IMPORTED_TARGET movit)
  find_package(OpenGL)
  if(UNIX AND NOT APPLE)
    find_package(X11)
    if(NOT TARGET X11::X11)
      set(MOD_OPENGL OFF)
    endif()
  endif()
endif()

if(MOD_PLUS)
  pkg_check_modules(libebur128 IMPORTED_TARGET libebur128)
endif()

if(MOD_RESAMPLE)
  pkg_check_modules(samplerate IMPORTED_TARGET samplerate)
endif()

if(MOD_RTAUDIO)
  pkg_check_modules(rtaudio IMPORTED_TARGET rtaudio)
  if(NOT TARGET PkgConfig::rtaudio AND UNIX AND NOT APPLE)
    pkg_check_modules(alsa IMPORTED_TARGET alsa)
    pkg_check_modules(libpulse-simple IMPORTED_TARGET libpulse-simple)
  endif()
endif()

if(MOD_RUBBERBAND)
  pkg_check_modules(rubberband IMPORTED_TARGET rubberband)
endif()

if(MOD_SDL1)
  pkg_check_modules(sdl IMPORTED_TARGET sdl)
  pkg_check_modules(sdl_image IMPORTED_TARGET SDL_image)
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

if(SWIG_CSHARP)
  find_package(Mono)
endif()

if(SWIG_JAVA)
  find_package(JNI)
endif()

if(SWIG_LUA)
  find_package(Lua)
endif()

if(SWIG_NODEJS)
  find_package(Node)
  if(NODE_VERSION_MAJOR VERSION_GREATER_EQUAL 12 AND SWIG_VERSION VERSION_LESS 4.1)
    # https://github.com/swig/swig/issues/1520
    set(SWIG_NODEJS OFF)
  endif()
endif()

if(SWIG_PERL)
  find_package(PerlLibs)
endif()

if(SWIG_PHP)
  find_package(PHP)
endif()

if(SWIG_PYTHON)
  find_package(Python3 COMPONENTS Interpreter Development)
endif()

if(SWIG_RUBY)
  find_package(Ruby)
endif()

if(SWIG_TCL)
  find_package(TCL)
endif()

if(BUILD_DOCS)
  find_package(Doxygen REQUIRED)

  set(DOXYGEN_OUTPUT_DIRECTORY "docs")
  set(DOXYGEN_ABBREVIATE_BRIEF "")
  set(DOXYGEN_STRIP_FROM_PATH "src/framework/")
  set(DOXYGEN_JAVADOC_AUTOBRIEF "YES")
  set(DOXYGEN_QT_AUTOBRIEF "YES")
  set(DOXYGEN_ALIASES "properties=\\xrefitem properties \\\"Property\\\" \\\"Properties Dictionary\\\"")
  list(APPEND DOXYGEN_ALIASES "event=\\xrefitem event \\\"Event\\\" \\\"Events Dictionary\\\"")
  list(APPEND DOXYGEN_ALIASES "envvar=\\xrefitem envvars \\\"Environment Variable\\\" \\\"Environment Variables\\\"")
  set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C "YES")
  set(DOXYGEN_SORT_BRIEF_DOCS "YES")
  set(DOXYGEN_EXTRACT_ALL "YES")
  set(DOXYGEN_EXTRACT_PRIVATE "YES")
  set(DOXYGEN_EXTRACT_STATIC "YES")
  set(DOXYGEN_FILE_PATTERNS "")
  set(DOXYGEN_RECURSIVE "NO")
  set(DOXYGEN_EXCLUDE_PATTERNS "")
  set(DOXYGEN_EXAMPLE_PATTERNS "")
  set(DOXYGEN_SEARCHENGINE "NO")

  doxygen_add_docs(docs src/framework)
endif()

install(DIRECTORY presets profiles DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt)

if(UNIX AND NOT APPLE)
  install(FILES docs/melt.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()

add_subdirectory(src)
