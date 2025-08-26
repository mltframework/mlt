add_library(mltrubberband MODULE factory.c filter_rbpitch.cpp)

file(GLOB YML "*.yml")
add_custom_target(Other_rubberband_Files SOURCES
  ${YML}
)
include(GenerateExportHeader)
generate_export_header(mltrubberband
  EXPORT_MACRO_NAME MLTRUBBERBAND_EXPORT
)
target_compile_options(mltrubberband PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltrubberband PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltrubberband PRIVATE mlt PkgConfig::rubberband)

set_target_properties(mltrubberband PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltrubberband LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_rbpitch.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/rubberband)
