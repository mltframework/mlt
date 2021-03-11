add_library(mltvidstab MODULE
  common.c
  factory.c
  filter_deshake.cpp
  filter_vidstab.cpp
)

target_compile_options(mltvidstab PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltvidstab PRIVATE mlt m mlt++ PkgConfig::vidstab)

set_target_properties(mltvidstab PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltvidstab LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_deshake.yml filter_vidstab.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/vidstab)
