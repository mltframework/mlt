add_library(mltvidstab MODULE
  common.c common.h
  factory.c
  filter_deshake.cpp
  filter_vidstab.cpp
)
include(GenerateExportHeader)
generate_export_header(mltvidstab)
file(GLOB YML "*.yml")
add_custom_target(Other_vidstab_Files SOURCES
  ${YML}
)

target_compile_options(mltvidstab PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltvidstab PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltvidstab PRIVATE mlt mlt++ PkgConfig::vidstab)
if(NOT MSVC)
  target_link_libraries(mltvidstab PRIVATE m)
endif()

set_target_properties(mltvidstab PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltvidstab LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_deshake.yml filter_vidstab.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/vid.stab)
