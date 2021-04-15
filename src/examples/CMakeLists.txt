cmake_minimum_required(VERSION 3.14)

project(MLT-examples)

option(WITH_PKGCONFIG "Use pkg-config to find MLT" OFF)

add_executable(play play.cpp)

if(WITH_PKGCONFIG)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(mlt++ REQUIRED IMPORTED_TARGET mlt++-7)
  target_link_libraries(play PRIVATE PkgConfig::mlt++)
else()
  find_package(Mlt7 REQUIRED COMPONENTS avformat sdl)
  target_link_libraries(play PRIVATE Mlt7::mlt++)
endif()
