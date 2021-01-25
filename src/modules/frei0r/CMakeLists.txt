file(GLOB mltfrei0r_src *.c)
add_library(mltfrei0r MODULE ${mltfrei0r_src})
target_link_libraries(mltfrei0r mlt m ${CMAKE_DL_LIBS})
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltfrei0r PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltfrei0r LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES blacklist.txt not_thread_safe.txt param_name_map.yaml ${yml}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/frei0r)
