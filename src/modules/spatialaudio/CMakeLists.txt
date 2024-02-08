add_library(mltspatialaudio MODULE factory.c filter_ambisonic-decoder.cpp)

file(GLOB YML "*.yml")
add_custom_target(Other_spatialaudio_Files SOURCES
  ${YML}
)

target_compile_options(mltspatialaudio PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltspatialaudio PRIVATE mlt PkgConfig::spatialaudio)

set_target_properties(mltspatialaudio PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltspatialaudio LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_ambisonic-decoder.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/spatialaudio)
