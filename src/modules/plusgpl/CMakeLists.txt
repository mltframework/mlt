add_library(mltplusgpl MODULE
  cJSON.c
  consumer_cbrts.c
  factory.c
  filter_burn.c
  filter_lumaliftgaingamma.c
  filter_rotoscoping.c
  filter_telecide.c
  image.c
  utils.c
)

target_link_libraries(mltplusgpl mlt m Threads::Threads)

if(WIN32)
  target_link_libraries(mltplusgpl ws2_32)
endif()

# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltplusgpl PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)

install(TARGETS mltplusgpl LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES
  consumer_cbrts.yml
  filter_burningtv.yml
  filter_lumaliftgaingamma.yml
  filter_rotoscoping.yml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/plusgpl
)
