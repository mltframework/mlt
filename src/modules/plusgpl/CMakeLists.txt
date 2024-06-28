add_library(mltplusgpl MODULE
  cJSON.c cJSON.h
  consumer_cbrts.c
  factory.c
  filter_burn.c
  filter_lumaliftgaingamma.c
  filter_rotoscoping.c
  filter_telecide.c
  image.c
  utils.c utils.h
)

file(GLOB YML "*.yml")
add_custom_target(Other_plsugpl_Files SOURCES
  ${YML}
)

target_compile_options(mltplusgpl PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltplusgpl PRIVATE mlt m Threads::Threads)

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
  filter_rotoscoping.yml
  DESTINATION ${MLT_INSTALL_DATA_DIR}/plusgpl
)
