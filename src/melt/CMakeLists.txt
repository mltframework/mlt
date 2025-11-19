add_executable(melt melt.c io.c io.h)

target_compile_options(melt PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(melt PRIVATE mlt Threads::Threads)

target_compile_definitions(melt PRIVATE VERSION="${MLT_VERSION}")

if(SDL2_FOUND AND NOT ANDROID)
    target_link_libraries(melt PRIVATE SDL2::SDL2)
    target_compile_definitions(melt PRIVATE HAVE_SDL2)
    if(MINGW)
        target_link_libraries(melt PRIVATE ${MLT_PTHREAD_LIBS})
        target_compile_definitions(melt PRIVATE main=SDL_main)
        target_link_libraries(melt PRIVATE mingw32 SDL2main SDL2)
    endif()
endif()

if(MSVC)
  target_link_libraries(melt PRIVATE msvccompat)
endif()

if(MINGW)
  target_link_options(melt PRIVATE -mconsole)
endif()

if(UNIX AND NOT APPLE)
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E remove melt melt-${MLT_VERSION_MAJOR} \
                                  WORKING_DIRECTORY \$ENV\{DESTDIR\}${CMAKE_INSTALL_FULL_BINDIR})"
    )
endif()

install(TARGETS melt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(UNIX AND NOT APPLE)
  install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E rename melt melt-${MLT_VERSION_MAJOR} \
                                WORKING_DIRECTORY \$ENV\{DESTDIR\}${CMAKE_INSTALL_FULL_BINDIR})"
  )
  install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink melt-${MLT_VERSION_MAJOR} melt \
                                WORKING_DIRECTORY \$ENV\{DESTDIR\}${CMAKE_INSTALL_FULL_BINDIR})"
  )
endif()
