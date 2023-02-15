add_library(mltavformat MODULE
  common.c common.h
  factory.c
  filter_avcolour_space.c
  filter_avdeinterlace.c
  filter_swscale.c
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
  m
  Threads::Threads
  PkgConfig::libavformat
  PkgConfig::libswscale
  PkgConfig::libavutil
)

target_compile_definitions(mltavformat PRIVATE FILTERS)

if(WIN32)
  target_compile_definitions(mltavformat PRIVATE AVDATADIR="share/ffmpeg/")
endif()

if(TARGET PkgConfig::libavcodec)
  target_sources(mltavformat PRIVATE producer_avformat.c consumer_avformat.c)
  target_link_libraries(mltavformat PRIVATE PkgConfig::libavcodec)
  target_compile_definitions(mltavformat PRIVATE CODECS)
endif()

if(TARGET PkgConfig::libavfilter)
  target_sources(mltavformat PRIVATE filter_avfilter.c link_avdeinterlace.c)
  target_link_libraries(mltavformat PRIVATE PkgConfig::libavfilter)
  target_compile_definitions(mltavformat PRIVATE AVFILTER)
endif()

if(TARGET PkgConfig::libavdevice)
  target_link_libraries(mltavformat PRIVATE PkgConfig::libavdevice)
  target_compile_definitions(mltavformat PRIVATE AVDEVICE)
endif()

if(TARGET PkgConfig::libswresample)
  target_sources(mltavformat PRIVATE common_swr.c common_swr.h filter_swresample.c link_swresample.c)
  target_link_libraries(mltavformat PRIVATE PkgConfig::libswresample)
  target_compile_definitions(mltavformat PRIVATE SWRESAMPLE)
endif()

if(CPU_MMX)
  target_compile_definitions(mltavformat PRIVATE USE_MMX)
endif()

set_target_properties(mltavformat PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltavformat LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  consumer_avformat.yml
  producer_avformat.yml
  resolution_scale.yml
  blacklist.txt
  yuv_only.txt
  DESTINATION ${MLT_INSTALL_DATA_DIR}/avformat
)
