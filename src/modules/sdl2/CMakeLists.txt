add_library(mltsdl2 MODULE
  common.c
  consumer_sdl2_audio.c
  consumer_sdl2.c
  factory.c
)

target_compile_options(mltsdl2 PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltsdl2 PRIVATE mlt m Threads::Threads sdl2)

set_target_properties(mltsdl2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltsdl2 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES consumer_sdl2_audio.yml consumer_sdl2.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/sdl2)
