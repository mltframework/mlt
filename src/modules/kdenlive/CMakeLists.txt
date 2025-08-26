add_library(mltkdenlive MODULE
  factory.c
  filter_boxblur.c
  filter_freeze.c
  filter_wave.c
  producer_framebuffer.c
)

file(GLOB YML "*.yml")
add_custom_target(Other_kdenlive_Files SOURCES
  ${YML}
)
include(GenerateExportHeader)
generate_export_header(mltkdenlive)
target_compile_options(mltkdenlive PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltkdenlive PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltkdenlive PRIVATE mlt)
if(NOT MSVC)
  target_link_libraries(mltkdenlive PRIVATE m)
endif()

set_target_properties(mltkdenlive PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltkdenlive LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  filter_boxblur.yml
  filter_freeze.yml
  filter_wave.yml
  producer_framebuffer.yml
  DESTINATION ${MLT_INSTALL_DATA_DIR}/kdenlive
)
