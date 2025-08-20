add_library(mltvidstab MODULE
  common.c common.h
  factory.c
  filter_deshake.cpp
  filter_vidstab.cpp
)

file(GLOB YML "*.yml")
add_custom_target(Other_vidstab_Files SOURCES
  ${YML}
)

target_compile_options(mltvidstab PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltvidstab PRIVATE mlt mlt++ PkgConfig::vidstab)
if(MSVC)
  target_link_libraries(mltvidstab PRIVATE msvccompat PThreads4W::PThreads4W)
  # Add vid.stab headers from external directory for MSVC
  target_include_directories(mltvidstab PRIVATE ${CMAKE_SOURCE_DIR}/external/vid.stab/src)
  mlt_add_vcpkg_include_dirs(mltvidstab)
else()
  target_link_libraries(mltvidstab PRIVATE m)
endif()

set_target_properties(mltvidstab PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltvidstab LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_deshake.yml filter_vidstab.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/vid.stab)
