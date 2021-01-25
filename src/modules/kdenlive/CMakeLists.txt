file(GLOB mltkdenlive_src *.c)
add_library(mltkdenlive MODULE ${mltkdenlive_src})
target_link_libraries(mltkdenlive mlt m)
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltkdenlive PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltkdenlive LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/kdenlive)
