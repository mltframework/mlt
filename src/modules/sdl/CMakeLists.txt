add_library(mltsdl MODULE
  consumer_sdl_audio.c
  consumer_sdl_preview.c
  consumer_sdl_still.c
  consumer_sdl.c
  factory.c
)

target_compile_options(mltsdl PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltsdl PRIVATE mlt m Threads::Threads PkgConfig::sdl)

if(TARGET PkgConfig::sdl_image)
  target_sources(mltsdl PRIVATE producer_sdl_image.c)
  target_link_libraries(mltsdl PRIVATE PkgConfig::sdl_image)
  target_compile_definitions(mltsdl PRIVATE WITH_SDL_IMAGE)
  install(FILES producer_sdl_image.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/sdl)
endif()

set_target_properties(mltsdl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltsdl LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  consumer_sdl_audio.yml
  consumer_sdl_preview.yml
  consumer_sdl_still.yml
  consumer_sdl.yml
  DESTINATION ${MLT_INSTALL_DATA_DIR}/sdl
)
