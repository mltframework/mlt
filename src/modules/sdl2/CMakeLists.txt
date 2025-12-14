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
include(GenerateExportHeader)
generate_export_header(mltsdl2)
target_compile_options(mltsdl2 PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltsdl2 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltsdl2 PRIVATE mlt Threads::Threads SDL2::SDL2)
if(MSVC)
  target_link_libraries(mltsdl2 PRIVATE PThreads4W::PThreads4W msvccompat)
else()
  target_link_libraries(mltsdl2 PRIVATE m)
endif()

if(MINGW)
  target_link_libraries(mltsdl2 PRIVATE ${MLT_PTHREAD_LIBS})
endif()

set_target_properties(mltsdl2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltsdl2 LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES consumer_sdl2_audio.yml consumer_sdl2.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/sdl2)
