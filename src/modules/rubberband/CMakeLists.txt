add_library(mltrubberband MODULE factory.c filter_rbpitch.cpp)
target_link_libraries(mltrubberband PRIVATE mlt PkgConfig::rubberband)

set_target_properties(mltrubberband PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltrubberband LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES filter_rbpitch.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/rubberband)
