add_library(mltsdl MODULE
  consumer_sdl_audio.c
  consumer_sdl_preview.c
  consumer_sdl_still.c
  consumer_sdl.c
  factory.c
)

file(GLOB YML "*.yml")
add_custom_target(Other_sdl_Files SOURCES
  ${YML}
)
include(GenerateExportHeader)
generate_export_header(mltsdl
  EXPORT_MACRO_NAME MLTSDL_EXPORT
)
target_compile_options(mltsdl PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltsdl PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltsdl PRIVATE mlt Threads::Threads PkgConfig::sdl)
if(NOT MSVC)
  target_link_libraries(mltsdl PRIVATE m)
endif()

if(APPLE)
  target_link_libraries(mltsdl PRIVATE objc "-framework Foundation")
elseif(UNIX)
  target_link_libraries(mltsdl PRIVATE X11)
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
