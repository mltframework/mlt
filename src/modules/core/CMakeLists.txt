add_library(mltcore MODULE
  consumer_multi.c
  consumer_null.c
  factory.c
  filter_audiochannels.c
  filter_audioconvert.c
  filter_audiomap.c
  filter_audioseam.c
  filter_audiowave.c
  filter_autofade.c
  filter_box_blur.c
  filter_brightness.c
  filter_channelcopy.c
  filter_choppy.c
  filter_crop.c
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
  filter_pillar_echo.c
  filter_rescale.c
  filter_resize.c
  filter_transition.c
  filter_watermark.c
  image_proc.c image_proc.h
  link_timeremap.c
  producer_blank.c
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
)

file(GLOB YML "*.yml")
add_custom_target(Other_core_Files SOURCES
  ${YML}
  loader.dict
  loader.ini
)

target_compile_options(mltcore PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltcore PRIVATE mlt Threads::Threads)

if(MSVC)
  target_link_libraries(mltcore PRIVATE PThreads4W::PThreads4W)
else()
  target_link_libraries(mltcore PRIVATE m)
endif()

if(WIN32)
  target_sources(mltcore PRIVATE ../../win32/fnmatch.c)
  target_include_directories(mltcore PRIVATE ../../win32)
endif()

if(CPU_SSE)
  target_compile_definitions(mltcore PRIVATE USE_SSE)
endif()

if(CPU_X86_64)
  target_sources(mltcore PRIVATE composite_line_yuv_sse2_simple.c)
  target_compile_definitions(mltcore PRIVATE ARCH_X86_64)
endif()

set_target_properties(mltcore PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltcore LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  ${YML}
  loader.dict
  loader.ini
  DESTINATION ${MLT_INSTALL_DATA_DIR}/core
)
