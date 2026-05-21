add_library(mltrnnoise MODULE factory.c filter_rnnoise.c link_rnnoise.c)

file(GLOB YML "*.yml")
add_custom_target(Other_rnnoise_Files SOURCES ${YML})

include(GenerateExportHeader)
generate_export_header(mltrnnoise)

target_compile_options(mltrnnoise PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltrnnoise PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltrnnoise PRIVATE mlt PkgConfig::rnnoise)

set_target_properties(mltrnnoise PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltrnnoise LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})
install(FILES filter_rnnoise.yml link_rnnoise.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/rnnoise)
