add_library(mltplus MODULE
  consumer_blipflash.c
  factory.c
  filter_affine.c
  filter_charcoal.c
  filter_dynamictext.c
  filter_dynamic_loudness.c
  filter_invert.c
  filter_lift_gamma_gain.c
  filter_loudness.c
  filter_loudness_meter.c
  filter_lumakey.c
  filter_pillar_echo.c
  filter_rgblut.c
  filter_sepia.c
  filter_spot_remover.c
  filter_text.c
  filter_timer.c
  filter_strobe.c
  producer_blipflash.c
  producer_count.c
  transition_affine.c
)

target_link_libraries(mltplus mlt m Threads::Threads)

if(TARGET PkgConfig::FFTW)
  target_sources(mltplus PRIVATE filter_dance.c filter_fft.c)
  target_link_libraries(mltplus PkgConfig::FFTW)
  target_compile_definitions(mltplus PRIVATE USE_FFTW)
  install(FILES filter_dance.yml filter_fft.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/plus)
endif()

if(TARGET PkgConfig::libebur128)
  target_link_libraries(mltplus PkgConfig::libebur128)
else()
  target_sources(mltplus PRIVATE ebur128/ebur128.c)
  target_include_directories(mltplus PRIVATE ebur128 ebur128/queue) 
  target_compile_definitions(mltplus PRIVATE USE_INTERNAL_LIBEBUR128)
endif()

set_target_properties(mltplus PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltplus LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES
  consumer_blipflash.yml
  filter_affine.yml
  filter_charcoal.yml
  filter_dynamic_loudness.yml
  filter_dynamictext.yml
  filter_invert.yml
  filter_lift_gamma_gain.yml
  filter_loudness_meter.yml
  filter_loudness.yml
  filter_lumakey.yml
  filter_pillar_echo.yml
  filter_rgblut.yml
  filter_sepia.yml
  filter_spot_remover.yml
  filter_strobe.yml
  filter_text.yml
  filter_timer.yml
  producer_blipflash.yml
  producer_count.yml
  transition_affine.yml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/plus
)
