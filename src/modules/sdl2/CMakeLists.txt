file(GLOB mltsdl2_src *.c)
add_library(mltsdl2 MODULE ${mltsdl2_src})
target_link_libraries(mltsdl2 mlt m Threads::Threads sdl2)
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltsdl2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltsdl2 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/sdl2)
