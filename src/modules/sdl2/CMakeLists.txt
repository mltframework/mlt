add_library(mltsdl2 MODULE
  common.c
  consumer_sdl2_audio.c
  consumer_sdl2.c
  factory.c
)

file(GLOB YML "*.yml")
add_custom_target(Other_sdl2_Files SOURCES
  ${YML}
)

target_compile_options(mltsdl2 PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltsdl2 PRIVATE mlt Threads::Threads SDL2::SDL2)
if(MSVC)
  target_link_libraries(mltsdl2 PRIVATE msvccompat PThreads4W::PThreads4W)
else()
  target_link_libraries(mltsdl2 PRIVATE m)
endif()

if(MINGW)
  target_link_libraries(mltsdl2 PRIVATE ${MLT_PTHREAD_LIBS})
endif()

set_target_properties(mltsdl2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltsdl2 LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES consumer_sdl2_audio.yml consumer_sdl2.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/sdl2)
