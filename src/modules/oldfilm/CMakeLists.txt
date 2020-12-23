if(NOT MOD_OLDFILM)
    return()
endif()

file(GLOB mltoldfilm_src *.c)
add_library(mltoldfilm MODULE ${mltoldfilm_src})
target_link_libraries(mltoldfilm mlt m)
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltoldfilm PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltoldfilm LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/oldfilm)
