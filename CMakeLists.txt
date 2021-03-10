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

set(MLT_INSTALL_MODULE_DIR ${CMAKE_INSTALL_LIBDIR}/mlt)
set(MLT_INSTALL_DATA_DIR ${CMAKE_INSTALL_DATADIR}/mlt)
if(NOT (WIN32 OR APPLE))
  set(MLT_INSTALL_MODULE_DIR ${CMAKE_INSTALL_LIBDIR}/mlt-${MLT_VERSION_MAJOR})
  set(MLT_INSTALL_DATA_DIR ${CMAKE_INSTALL_DATADIR}/mlt-${MLT_VERSION_MAJOR})
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

if(NOT GPL)
  set(MOD_MOTION_EST OFF)
  set(MOD_NORMALIZE OFF)
  set(MOD_PLUSGPL OFF)
  set(MOD_QT OFF)
  set(MOD_RUBBERBAND OFF)
  set(MOD_VIDSTAB OFF)
  set(MOD_XINE OFF)
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
  if(TARGET PkgConfig::libavformat AND TARGET PkgConfig::libswscale AND TARGET PkgConfig::libavutil)
    pkg_check_modules(libavcodec IMPORTED_TARGET libavcodec)
    pkg_check_modules(libavfilter IMPORTED_TARGET libavfilter)
    pkg_check_modules(libavdevice IMPORTED_TARGET libavdevice)
    pkg_check_modules(libswresample IMPORTED_TARGET libswresample)
    list(APPEND MLT_SUPPORTED_COMPONENTS avformat)
  else()
    set(MOD_AVFORMAT OFF)
  endif()
endif()

if(MOD_DECKLINK)
  list(APPEND MLT_SUPPORTED_COMPONENTS decklink)
endif()

if(MOD_FREI0R)
  pkg_check_modules(FREI0R frei0r)
  if(FREI0R_FOUND)
    list(APPEND MLT_SUPPORTED_COMPONENTS frei0r)
  else()
    set(MOD_FREI0R OFF)
  endif()
endif()

if(MOD_GDK)
  pkg_check_modules(GdkPixbuf IMPORTED_TARGET gdk-pixbuf-2.0)
  if(TARGET PkgConfig::GdkPixbuf)
    pkg_check_modules(pango IMPORTED_TARGET pango)
    pkg_check_modules(pangoft2 IMPORTED_TARGET pangoft2)
    list(APPEND MLT_SUPPORTED_COMPONENTS gdk)
  else()
    set(MOD_GDK OFF)
  endif()
endif()

if(MOD_JACKRACK)
  pkg_check_modules(jack IMPORTED_TARGET jack)
  if(TARGET PkgConfig::jack)
    pkg_check_modules(glib IMPORTED_TARGET glib-2.0)
    check_include_file(ladspa.h ladspa_h_FOUND)
    list(APPEND MLT_SUPPORTED_COMPONENTS jackrack)
  else()
    set(MOD_JACKRACK OFF)
  endif()
endif()

if(MOD_KDENLIVE)
  list(APPEND MLT_SUPPORTED_COMPONENTS kdenlive)
endif()

if(MOD_MOTION_EST)
  list(APPEND MLT_SUPPORTED_COMPONENTS motion_est)
endif()

if(MOD_NDI)
  find_package(NDI REQUIRED)
  list(APPEND MLT_SUPPORTED_COMPONENTS ndi)
endif()

if(MOD_NORMALIZE)
  list(APPEND MLT_SUPPORTED_COMPONENTS normalize)
endif()

if(MOD_OLDFILM)
  list(APPEND MLT_SUPPORTED_COMPONENTS oldfilm)
endif()

if(MOD_OPENCV)
  find_package(OpenCV REQUIRED COMPONENTS tracking)
  if(OpenCV_tracking_FOUND)
    list(APPEND MLT_SUPPORTED_COMPONENTS opencv)
  else()
    set(MOD_OPENCV OFF)
  endif()
endif()

if(MOD_OPENGL)
  pkg_check_modules(movit IMPORTED_TARGET movit)
  find_package(OpenGL)
  if(TARGET PkgConfig::movit AND TARGET OpenGL::GL)
    if(UNIX AND NOT APPLE)
      find_package(X11 REQUIRED)
    endif()
    list(APPEND MLT_SUPPORTED_COMPONENTS opengl)
  else()
    set(MOD_OPENGL OFF)
  endif()
endif()

if(MOD_PLUS)
  pkg_check_modules(libebur128 IMPORTED_TARGET libebur128)
  list(APPEND MLT_SUPPORTED_COMPONENTS plus)
endif()

if(MOD_PLUSGPL)
  list(APPEND MLT_SUPPORTED_COMPONENTS plusgpl)
endif()

if(MOD_QT)
  find_package(Qt5 COMPONENTS Core Gui Xml Svg Widgets)
  if(Qt5_FOUND)
    list(APPEND MLT_SUPPORTED_COMPONENTS qt)
  else()
    set(MOD_QT OFF)
  endif()
endif()

if(MOD_RESAMPLE)
  pkg_check_modules(samplerate IMPORTED_TARGET samplerate)
  if(TARGET PkgConfig::samplerate)
    list(APPEND MLT_SUPPORTED_COMPONENTS resample)
  else()
    set(MOD_RESAMPLE OFF)
  endif()
endif()

if(MOD_RTAUDIO)
  pkg_check_modules(rtaudio IMPORTED_TARGET rtaudio)
  if(NOT TARGET PkgConfig::rtaudio AND UNIX AND NOT APPLE)
    pkg_check_modules(alsa IMPORTED_TARGET alsa)
    pkg_check_modules(libpulse-simple IMPORTED_TARGET libpulse-simple)
  endif()
  list(APPEND MLT_SUPPORTED_COMPONENTS rtaudio)
endif()

if(MOD_RUBBERBAND)
  pkg_check_modules(rubberband IMPORTED_TARGET rubberband)
  if(TARGET PkgConfig::rubberband)
    list(APPEND MLT_SUPPORTED_COMPONENTS rubberband)
  else()
    set(MOD_RUBBERBAND OFF)
  endif()
endif()

if(MOD_SDL1)
  pkg_check_modules(sdl IMPORTED_TARGET sdl)
  if(TARGET PkgConfig::sdl)
    pkg_check_modules(sdl_image IMPORTED_TARGET SDL_image)
    list(APPEND MLT_SUPPORTED_COMPONENTS sdl)
  else()
    set(MOD_SDL1 OFF)
  endif()
endif()

if(MOD_SDL2)
  if(TARGET sdl2)
    list(APPEND MLT_SUPPORTED_COMPONENTS sdl2)
  else()
    set(MOD_SDL2 OFF)
  endif()
endif()

if(MOD_SOX)
  pkg_check_modules(sox IMPORTED_TARGET sox)
  if(TARGET PkgConfig::sox)
    list(APPEND MLT_SUPPORTED_COMPONENTS sox)
  else()
    set(MOD_SOX OFF)
  endif()
endif()

if(MOD_VIDSTAB)
  pkg_check_modules(vidstab IMPORTED_TARGET vidstab)
  if(TARGET PkgConfig::vidstab)
    list(APPEND MLT_SUPPORTED_COMPONENTS vidstab)
  else()
    set(MOD_VIDSTAB OFF)
  endif()
endif()

if(MOD_VMFX)
  list(APPEND MLT_SUPPORTED_COMPONENTS vmfx)
endif()

if(MOD_VORBIS)
  pkg_check_modules(vorbis IMPORTED_TARGET vorbis)
  pkg_check_modules(vorbisfile IMPORTED_TARGET vorbisfile)
  if(TARGET PkgConfig::vorbis AND TARGET PkgConfig::vorbisfile)
    list(APPEND MLT_SUPPORTED_COMPONENTS vorbis)
  else()
    set(MOD_VORBIS OFF)
  endif()
endif()

if(MOD_XINE)
  list(APPEND MLT_SUPPORTED_COMPONENTS xine)
endif()

if(MOD_XML)
  if(TARGET PkgConfig::xml)
    list(APPEND MLT_SUPPORTED_COMPONENTS xml)
  else()
    set(MOD_XML OFF)
  endif()
endif()

find_package(SWIG)

if(SWIG_CSHARP)
  find_package(Mono REQUIRED)
endif()

if(SWIG_JAVA)
  find_package(JNI REQUIRED)
endif()

if(SWIG_LUA)
  find_package(Lua REQUIRED)
endif()

if(SWIG_NODEJS)
  find_package(Node REQUIRED)
  if(NODE_VERSION_MAJOR VERSION_GREATER_EQUAL 12 AND SWIG_VERSION VERSION_LESS 4.1)
    # https://github.com/swig/swig/issues/1520
    set(SWIG_NODEJS OFF)
  endif()
endif()

if(SWIG_PERL)
  find_package(PerlLibs REQUIRED)
endif()

if(SWIG_PHP)
  find_package(PHP REQUIRED)
endif()

if(SWIG_PYTHON)
  find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
endif()

if(SWIG_RUBY)
  find_package(Ruby REQUIRED)
endif()

if(SWIG_TCL)
  find_package(TCL REQUIRED)
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

install(DIRECTORY presets profiles DESTINATION ${MLT_INSTALL_DATA_DIR})

if(UNIX AND NOT APPLE)
  install(FILES docs/melt.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 RENAME melt-${MLT_VERSION_MAJOR}.1)
  install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink \
                ${CMAKE_INSTALL_FULL_MANDIR}/man1/melt-${MLT_VERSION_MAJOR}.1 ${CMAKE_INSTALL_FULL_MANDIR}/man1/melt.1)"
  )
endif()

add_subdirectory(src)

install(EXPORT MltTargets
  FILE Mlt${MLT_VERSION_MAJOR}Targets.cmake
  NAMESPACE Mlt${MLT_VERSION_MAJOR}::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Mlt${MLT_VERSION_MAJOR}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/MltConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/Mlt${MLT_VERSION_MAJOR}Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Mlt${MLT_VERSION_MAJOR}
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/Mlt${MLT_VERSION_MAJOR}ConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/Mlt${MLT_VERSION_MAJOR}Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/Mlt${MLT_VERSION_MAJOR}ConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Mlt${MLT_VERSION_MAJOR}
)
