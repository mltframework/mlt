add_library(mltwsola MODULE factory.c filter_wsola.cpp wsola_algorithm.cpp)

file(GLOB YML "*.yml")
add_custom_target(Other_wsola_Files SOURCES
  ${YML}
)
include(GenerateExportHeader)
generate_export_header(mltwsola)
target_compile_options(mltwsola PRIVATE ${MLT_COMPILE_OPTIONS})
# Keep the floating point behavior of the Chromium original (no -ffast-math).
if(NOT MSVC)
  target_compile_options(mltwsola PRIVATE -fno-fast-math)
endif()
target_include_directories(mltwsola PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltwsola PRIVATE mlt)

set_target_properties(mltwsola PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltwsola LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_wsola.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/wsola)
