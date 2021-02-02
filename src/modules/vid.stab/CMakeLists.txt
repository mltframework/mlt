add_library(mltvidstab MODULE
  common.c
  factory.c
  filter_deshake.cpp
  filter_vidstab.cpp
)

target_link_libraries(mltvidstab mlt m mlt++ PkgConfig::vidstab)

# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltvidstab PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)

install(TARGETS mltvidstab LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES filter_deshake.yml filter_vidstab.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/vidstab)
