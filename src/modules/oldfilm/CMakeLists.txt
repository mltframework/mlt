add_library(mltoldfilm MODULE
  common.c common.h
  factory.c
  filter_dust.c
  filter_grain.c
  filter_lines.c
  filter_oldfilm.c
  filter_tcolor.c
  filter_vignette.c
)

file(GLOB YML "*.yml")
add_custom_target(Other_oldfilm_Files SOURCES
  ${YML}
)

target_compile_options(mltoldfilm PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltoldfilm PRIVATE mlt)
if(NOT MSVC)
  target_link_libraries(mltoldfilm PRIVATE m)
endif()

set_target_properties(mltoldfilm PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltoldfilm LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

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
  DESTINATION ${MLT_INSTALL_DATA_DIR}/oldfilm
)
