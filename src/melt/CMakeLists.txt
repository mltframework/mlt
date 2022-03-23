add_executable(melt melt.c io.c JitControl.pb-c.c JitStatus.pb-c.c)

target_compile_options(melt PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(melt PRIVATE mlt Threads::Threads protobuf-c)

target_compile_definitions(melt PRIVATE VERSION="${MLT_VERSION}")

if(TARGET PkgConfig::sdl2)
    target_link_libraries(melt PRIVATE PkgConfig::sdl2)
    target_compile_definitions(melt PRIVATE HAVE_SDL2)
    if(MINGW)
        target_link_libraries(melt PRIVATE mingw32)
    endif()
endif()

if(MINGW)
  target_link_options(melt PRIVATE -mconsole)
endif()

if(WIN32 OR APPLE)
  install(TARGETS melt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
  install(PROGRAMS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/melt" DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME melt-${MLT_VERSION_MAJOR})
  install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink melt-${MLT_VERSION_MAJOR} melt \
                                WORKING_DIRECTORY \$ENV\{DESTDIR\}${CMAKE_INSTALL_FULL_BINDIR})"
  )
endif()
