file(GLOB mltoldfilm_src *.c)
add_library(mltoldfilm MODULE ${mltoldfilm_src})
target_link_libraries(mltoldfilm mlt m)
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltoldfilm PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltoldfilm LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB DATA_FILES *.yml *.svg)
install(FILES ${DATA_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/oldfilm)
