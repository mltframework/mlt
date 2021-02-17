add_library(mltsox MODULE factory.c filter_sox.c)
target_link_libraries(mltsox mlt m PkgConfig::sox)

if(${sox_VERSION} GREATER 13)
    target_compile_definitions(mltsox PRIVATE SOX14)
endif()

set_target_properties(mltsox PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltsox LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES filter_sox_effect.yml filter_sox.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/sox)
