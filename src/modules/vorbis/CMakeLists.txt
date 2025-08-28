add_library(mltvorbis MODULE factory.c producer_vorbis.c)

file(GLOB YML "*.yml")
add_custom_target(Other_vorbis_Files SOURCES
  ${YML}
)
include(GenerateExportHeader)
generate_export_header(mltvorbis)
target_compile_options(mltvorbis PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltvorbis PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltvorbis PRIVATE mlt PkgConfig::vorbis PkgConfig::vorbisfile)

set_target_properties(mltvorbis PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltvorbis LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES producer_vorbis.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/vorbis)
