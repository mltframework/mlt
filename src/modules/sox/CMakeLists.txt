add_library(mltsox MODULE factory.c filter_sox.c)

file(GLOB YML "*.yml")
add_custom_target(Other_sox_Files SOURCES
  ${YML}
)

target_compile_options(mltsox PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltsox PRIVATE mlt PkgConfig::sox)
if(NOT MSVC)
  target_link_libraries(mltsox PRIVATE m)
else()
  target_compile_definitions(mltsox PRIVATE mlt_EXPORTS)
endif()

if(${sox_VERSION} GREATER 13)
    target_compile_definitions(mltsox PRIVATE SOX14)
endif()

set_target_properties(mltsox PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltsox LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_sox_effect.yml filter_sox.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/sox)
