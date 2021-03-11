add_library(mltnormalize MODULE
  factory.c
  filter_audiolevel.c
  filter_volume.c
)

target_compile_options(mltnormalize PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltnormalize PRIVATE mlt m)

set_target_properties(mltnormalize PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltnormalize LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  filter_audiolevel.yml
  filter_volume.yml
  DESTINATION ${MLT_INSTALL_DATA_DIR}/normalize
)
