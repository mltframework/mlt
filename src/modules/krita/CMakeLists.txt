add_library(mltkrita MODULE
)

target_compile_options(mltkrita PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltkrita PRIVATE mlt m)

set_target_properties(mltkrita PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltkrita LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  DESTINATION ${MLT_INSTALL_DATA_DIR}/krita
)

