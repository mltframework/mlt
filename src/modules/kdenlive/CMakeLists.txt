add_library(mltkdenlive MODULE
  factory.c
  filter_boxblur.c
  filter_freeze.c
  filter_wave.c
  producer_framebuffer.c
)

target_link_libraries(mltkdenlive PRIVATE mlt m)

set_target_properties(mltkdenlive PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltkdenlive LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES
  filter_boxblur.yml
  filter_freeze.yml
  filter_wave.yml
  producer_framebuffer.yml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/kdenlive
)
