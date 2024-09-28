add_library(mltopenfx MODULE
  factory.c
  mlt_openfx.c mlt_openfx.h
  filter_openfx.c
)

file(GLOB YML "*.yml")
add_custom_target(Other_openfx_Files SOURCES
  ${YML}
)

target_compile_options(mltopenfx PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltopenfx PRIVATE mlt m)

set_target_properties(mltopenfx PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltopenfx LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_openfx.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/openfx)
