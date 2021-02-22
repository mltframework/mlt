add_library(mltavformat MODULE
  common.c
  factory.c
  filter_avcolour_space.c
  filter_avdeinterlace.c
  filter_swscale.c
)

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
  target_sources(mltavformat PRIVATE filter_avfilter.c)
  target_link_libraries(mltavformat PRIVATE PkgConfig::libavfilter)
  target_compile_definitions(mltavformat PRIVATE AVFILTER)
endif()

if(TARGET PkgConfig::libavdevice)
  target_link_libraries(mltavformat PRIVATE PkgConfig::libavdevice)
  target_compile_definitions(mltavformat PRIVATE AVDEVICE)
endif()

if(TARGET PkgConfig::libswresample)
  target_sources(mltavformat PRIVATE filter_swresample.c)
  target_link_libraries(mltavformat PRIVATE PkgConfig::libswresample)
  target_compile_definitions(mltavformat PRIVATE SWRESAMPLE)
endif()

set_target_properties(mltavformat PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltavformat LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES
  consumer_avformat.yml
  producer_avformat.yml
  resolution_scale.yml
  blacklist.txt
  yuv_only.txt
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/avformat
)
