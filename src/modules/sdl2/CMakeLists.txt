add_library(mltsdl2 MODULE
  common.c
  consumer_sdl2_audio.c
  consumer_sdl2.c
  factory.c
)

target_compile_options(mltsdl2 PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltsdl2 PRIVATE mlt m Threads::Threads PkgConfig::sdl2)

set_target_properties(mltsdl2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltsdl2 LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES consumer_sdl2_audio.yml consumer_sdl2.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/sdl2)
