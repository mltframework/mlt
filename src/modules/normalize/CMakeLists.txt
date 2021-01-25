file(GLOB mltnormalize_src *.c)
add_library(mltnormalize MODULE ${mltnormalize_src})
target_link_libraries(mltnormalize mlt m)
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltnormalize PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltnormalize LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/normalize)
