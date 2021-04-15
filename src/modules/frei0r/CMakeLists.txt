add_library(mltfrei0r MODULE
  factory.c
  filter_cairoblend_mode.c
  filter_frei0r.c
  frei0r_helper.c
  producer_frei0r.c
  transition_frei0r.c
)

target_compile_options(mltfrei0r PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltfrei0r PRIVATE ${FREI0R_INCLUDE_DIRS})
if(APPLE AND RELOCATABLE)
  target_compile_definitions(mltfrei0r PRIVATE RELOCATABLE)
endif()

target_link_libraries(mltfrei0r PRIVATE mlt m ${CMAKE_DL_LIBS})

set_target_properties(mltfrei0r PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltfrei0r LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  filter_cairoblend_mode.yml
  resolution_scale.yml
  param_name_map.yaml
  blacklist.txt not_thread_safe.txt
  DESTINATION ${MLT_INSTALL_DATA_DIR}/frei0r
)
