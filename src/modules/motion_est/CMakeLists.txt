file(GLOB mltmotion_est_src *.c)
add_library(mltmotion_est MODULE ${mltmotion_est_src})
target_link_libraries(mltmotion_est mlt m)
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltmotion_est PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltmotion_est LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/motion_est)
