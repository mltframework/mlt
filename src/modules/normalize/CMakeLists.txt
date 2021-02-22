add_library(mltnormalize MODULE
  factory.c
  filter_audiolevel.c
  filter_volume.c
)

target_link_libraries(mltnormalize mlt m)

set_target_properties(mltnormalize PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltnormalize LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES
  filter_audiolevel.yml
  filter_volume.yml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/normalize
)
