file(GLOB mltrubberband_src *.c *.cpp)
add_library(mltrubberband MODULE ${mltrubberband_src})
target_link_libraries(mltrubberband mlt PkgConfig::rubberband)
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltrubberband PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltrubberband LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/rubberband)
