add_executable(melt melt.c io.c)
target_link_libraries(melt PRIVATE mlt Threads::Threads)
target_compile_definitions(melt PRIVATE VERSION="${MLT_VERSION}")

if(TARGET PkgConfig::sdl2)
    add_library(sdl2 ALIAS PkgConfig::sdl2)
    target_link_libraries(melt PRIVATE sdl2)
    target_compile_definitions(melt PRIVATE HAVE_SDL2)
    if(MINGW)
        target_link_libraries(melt PRIVATE mingw32)
    endif()
endif()

if(MINGW)
  target_link_options(melt PRIVATE -mconsole)
endif()

install(TARGETS melt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
