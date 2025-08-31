add_library(mltplusgpl MODULE
  cJSON.c cJSON.h
  consumer_cbrts.c
  factory.c
  filter_burn.c
  filter_lumaliftgaingamma.c
  filter_outline.cpp
  filter_rotoscoping.c
  filter_telecide.c
  image.c
  utils.c utils.h
)

file(GLOB YML "*.yml")
add_custom_target(Other_plsugpl_Files SOURCES
  ${YML}
)
include(GenerateExportHeader)
generate_export_header(mltplusgpl)
target_compile_options(mltplusgpl PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltplusgpl PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltplusgpl PRIVATE mlt mlt++ Threads::Threads)
if(NOT MSVC)
  target_link_libraries(mltplusgpl PRIVATE m)
else()
  target_link_libraries(mltplusgpl PRIVATE msvccompat)
endif()

if(WIN32)
  target_link_libraries(mltplusgpl PRIVATE ws2_32)
elseif(UNIX AND NOT APPLE AND NOT ANDROID AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
  target_link_libraries(mltplusgpl PRIVATE rt)
endif()

set_target_properties(mltplusgpl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltplusgpl LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  consumer_cbrts.yml
  filter_burningtv.yml
  filter_lumaliftgaingamma.yml
  filter_outline.yml
  filter_rotoscoping.yml
  DESTINATION ${MLT_INSTALL_DATA_DIR}/plusgpl
)
