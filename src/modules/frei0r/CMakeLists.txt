add_library(mltfrei0r MODULE
  factory.c
  filter_cairoblend_mode.c
  filter_frei0r.c
  frei0r_helper.c
  producer_frei0r.c
  transition_frei0r.c
)

file(GLOB YML "*.yml")
add_custom_target(Other_frei0r_Files SOURCES
  ${YML}
  aliases.yaml
  alpha_only.txt
  blacklist.txt
  filter_cairoblend_mode.yml
  not_thread_safe.txt
  param_name_map.yaml
  resolution_scale.yml
)

include(GenerateExportHeader)
generate_export_header(mltfrei0r)
target_compile_options(mltfrei0r PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltfrei0r PRIVATE ${FREI0R_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

if(RELOCATABLE)
  target_compile_definitions(mltfrei0r PRIVATE RELOCATABLE)
endif()

target_link_libraries(mltfrei0r PRIVATE mlt ${CMAKE_DL_LIBS})

if(NOT MSVC)
  target_link_libraries(mltfrei0r PRIVATE m)
endif()

set_target_properties(mltfrei0r PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltfrei0r LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  aliases.yaml
  alpha_only.txt
  blacklist.txt
  filter_cairoblend_mode.yml
  not_thread_safe.txt
  param_name_map.yaml
  resolution_scale.yml
  DESTINATION ${MLT_INSTALL_DATA_DIR}/frei0r
)
