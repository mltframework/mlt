add_library(mltfrei0r MODULE
  factory.c
  filter_cairoblend_mode.c
  filter_frei0r.c
  frei0r_helper.c
  producer_frei0r.c
  transition_frei0r.c
)

target_link_libraries(mltfrei0r mlt m ${CMAKE_DL_LIBS})
target_include_directories(mltfrei0r PRIVATE ${FREI0R_INCLUDE_DIRS})

# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltfrei0r PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)

install(TARGETS mltfrei0r LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES
  filter_cairoblend_mode.yml
  resolution_scale.yml
  param_name_map.yaml
  blacklist.txt not_thread_safe.txt
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/frei0r
)
