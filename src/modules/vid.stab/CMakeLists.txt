file(GLOB mltvidstab_src *.c *.cpp)
add_library(mltvidstab MODULE ${mltvidstab_src})
target_link_libraries(mltvidstab mlt m mlt++ PkgConfig::vidstab)
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltvidstab PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltvidstab LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/vidstab)
