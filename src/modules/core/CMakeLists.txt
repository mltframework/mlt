add_library(mltcore MODULE
  consumer_multi.c
  consumer_null.c
  factory.c
  filter_audiochannels.c
  filter_audioconvert.c
  filter_audiomap.c
  filter_audiowave.c
  filter_brightness.c
  filter_channelcopy.c
  filter_choppy.c
  filter_crop.c
  filter_data_feed.c
  filter_data_show.c
  filter_fieldorder.c
  filter_gamma.c
  filter_greyscale.c
  filter_imageconvert.c
  filter_luma.c
  filter_mask_apply.c
  filter_mask_start.c
  filter_mirror.c
  filter_mono.c
  filter_obscure.c
  filter_panner.c
  filter_region.c
  filter_rescale.c
  filter_resize.c
  filter_transition.c
  filter_watermark.c
  producer_colour.c
  producer_consumer.c
  producer_hold.c
  producer_loader.c
  producer_melt.c
  producer_noise.c
  producer_timewarp.c
  producer_tone.c
  transition_composite.c
  transition_luma.c
  transition_matte.c
  transition_mix.c
  transition_region.c
)

target_link_libraries(mltcore mlt Threads::Threads)

if(WIN32)
  target_sources(mltcore PRIVATE ../../win32/fnmatch.c)
endif()

if(X86_64)
  target_sources(mltcore PRIVATE composite_line_yuv_sse2_simple.c)
endif()

# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltcore PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)

install(TARGETS mltcore LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES
  consumer_multi.yml
  filter_audiomap.yml
  filter_audiowave.yml
  filter_brightness.yml
  filter_channelcopy.yml
  filter_choppy.yml
  filter_crop.yml
  filter_data_show.yml
  filter_fieldorder.yml
  filter_gamma.yml
  filter_greyscale.yml
  filter_luma.yml
  filter_mask_apply.yml
  filter_mask_start.yml
  filter_mirror.yml
  filter_mono.yml
  filter_obscure.yml
  filter_panner.yml
  filter_region.yml
  filter_rescale.yml
  filter_resize.yml
  filter_transition.yml
  filter_watermark.yml
  producer_colour.yml
  producer_consumer.yml
  producer_hold.yml
  producer_loader.yml
  producer_melt_file.yml
  producer_melt.yml
  producer_noise.yml
  producer_timewarp.yml
  producer_tone.yml
  transition_composite.yml
  transition_luma.yml
  transition_matte.yml
  transition_mix.yml
  transition_region.yml
  data_fx.properties
  loader.dict
  loader.ini
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/core
)
