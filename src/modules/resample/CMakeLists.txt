add_library(mltresample MODULE factory.c filter_resample.c link_resample.c)

file(GLOB YML "*.yml")
add_custom_target(Other_resample_Files SOURCES
  ${YML}
)
include(GenerateExportHeader)
generate_export_header(mltresample
  EXPORT_MACRO_NAME MLTRESAMPLE_EXPORT
)
target_compile_options(mltresample PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltresample PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltresample PRIVATE mlt PkgConfig::samplerate)

set_target_properties(mltresample PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltresample LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  filter_resample.yml
  link_resample.yml
  DESTINATION ${MLT_INSTALL_DATA_DIR}/resample
)
