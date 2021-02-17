add_library(mltoldfilm MODULE
  factory.c
  filter_dust.c
  filter_grain.c
  filter_lines.c
  filter_oldfilm.c
  filter_tcolor.c
  filter_vignette.c
)

target_link_libraries(mltoldfilm mlt m)

set_target_properties(mltoldfilm PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltoldfilm LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES
  filter_dust.yml
  filter_grain.yml
  filter_lines.yml
  filter_oldfilm.yml
  filter_tcolor.yml
  filter_vignette.yml
  dust1.svg
  dust2.svg
  dust3.svg
  dust4.svg
  dust5.svg
  fdust.svg
  grain.svg
  lines.svg
  oldfilm.svg
  tcolor.svg
  vignette.svg
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/oldfilm
)
