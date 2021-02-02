add_library(mltresample MODULE factory.c filter_resample.c)
target_link_libraries(mltresample mlt PkgConfig::samplerate)

# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltresample PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)

install(TARGETS mltresample LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES filter_resample.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/resample)
