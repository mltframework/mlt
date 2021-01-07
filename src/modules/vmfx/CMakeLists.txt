if(NOT MOD_VMFX)
    return()
endif()

file(GLOB mltvmfx_src *.c)
add_library(mltvmfx MODULE ${mltvmfx_src})
target_link_libraries(mltvmfx mlt)
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltvmfx PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltvmfx LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/vmfx)
