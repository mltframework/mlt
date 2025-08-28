add_library(mltnormalize MODULE
  factory.c
  filter_audiolevel.c
  filter_volume.c
)

file(GLOB YML "*.yml")
add_custom_target(Other_normalize_Files SOURCES
  ${YML}
)
include(GenerateExportHeader)
generate_export_header(mltnormalize)
target_compile_options(mltnormalize PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltnormalize PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltnormalize PRIVATE mlt)
if(NOT MSVC)
  target_link_libraries(mltnormalize PRIVATE m)
endif()

set_target_properties(mltnormalize PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltnormalize LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  filter_audiolevel.yml
  filter_volume.yml
  DESTINATION ${MLT_INSTALL_DATA_DIR}/normalize
)
