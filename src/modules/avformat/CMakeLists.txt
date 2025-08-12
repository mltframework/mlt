add_library(mltavformat MODULE
  common.c common.h
  factory.c
  filter_avcolour_space.c
  filter_avdeinterlace.c
  filter_swscale.c
  # avformat
  producer_avformat.c
  consumer_avformat.c
  # avfilter
  filter_avfilter.c
  link_avdeinterlace.c
  link_avfilter.c
  # swresample
  common_swr.c
  common_swr.h
  filter_swresample.c
  link_swresample.c
)

file(GLOB YML "*.yml")
add_custom_target(Other_avformat_Files SOURCES
  ${YML}
  blacklist.txt
  yuv_only.txt
)

target_compile_options(mltavformat PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltavformat PRIVATE
  mlt
  Threads::Threads
  ${FFMPEG_LIBRARIES}
)

if(NOT MSVC)
  target_link_libraries(mltavformat PRIVATE m)
endif()

if(WIN32)
  target_compile_definitions(mltavformat PRIVATE AVDATADIR="share/ffmpeg/")
  if(MINGW)
    target_link_libraries(mltavformat PRIVATE ${MLT_PTHREAD_LIBS})
  endif()
endif()

if(CPU_MMX)
  target_compile_definitions(mltavformat PRIVATE USE_MMX)
endif()

set_target_properties(mltavformat PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltavformat LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  consumer_avformat.yml
  filter_avcolour_space.yml
  filter_avdeinterlace.yml
  filter_swresample.yml
  filter_swscale.yml
  link_avdeinterlace.yml
  link_swresample.yml
  producer_avformat.yml
  resolution_scale.yml
  blacklist.txt
  yuv_only.txt
  DESTINATION ${MLT_INSTALL_DATA_DIR}/avformat
)
