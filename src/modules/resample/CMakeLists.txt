add_library(mltresample MODULE factory.c filter_resample.c link_resample.c)

file(GLOB YML "*.yml")
add_custom_target(Other_resample_Files SOURCES
  ${YML}
)

target_compile_options(mltresample PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltresample PRIVATE mlt PkgConfig::samplerate)

set_target_properties(mltresample PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltresample LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_resample.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/resample)
