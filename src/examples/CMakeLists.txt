cmake_minimum_required(VERSION 3.14)

project(MLT-examples)

find_package(PkgConfig REQUIRED)
pkg_check_modules(mlt++ REQUIRED IMPORTED_TARGET mlt++)

add_executable(play play.cpp)
target_link_libraries(play PRIVATE PkgConfig::mlt++)
