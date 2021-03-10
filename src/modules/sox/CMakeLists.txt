add_library(mltsox MODULE factory.c filter_sox.c)

target_compile_options(mltsox PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltsox PRIVATE mlt m PkgConfig::sox)

if(${sox_VERSION} GREATER 13)
    target_compile_definitions(mltsox PRIVATE SOX14)
endif()

set_target_properties(mltsox PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltsox LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_sox_effect.yml filter_sox.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/sox)
