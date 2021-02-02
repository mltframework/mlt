add_library(mltrubberband MODULE factory.c filter_rbpitch.cpp)
target_link_libraries(mltrubberband mlt PkgConfig::rubberband)

# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltrubberband PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)

install(TARGETS mltrubberband LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES filter_rbpitch.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/rubberband)
