set(mltavformat_libs
    mlt m Threads::Threads
    PkgConfig::libavformat
    PkgConfig::libswscale
    PkgConfig::libavutil)
set(mltavformat_defs FILTERS)
if(WIN32)
    list(APPEND mltavformat_defs AVDATADIR="share/ffmpeg/")
endif()
set(mltavformat_srcs
    common.c
    factory.c
    filter_avcolour_space.c
    filter_avdeinterlace.c
    filter_swscale.c)
if(TARGET PkgConfig::libavcodec)
    list(APPEND mltavformat_libs PkgConfig::libavcodec)
    list(APPEND mltavformat_defs CODECS)
    list(APPEND mltavformat_srcs
        producer_avformat.c
        consumer_avformat.c)
endif()
if(TARGET PkgConfig::libavfilter)
    list(APPEND mltavformat_libs PkgConfig::libavfilter)
    list(APPEND mltavformat_defs FILTERS AVFILTER )
    list(APPEND mltavformat_srcs
        filter_avfilter.c)
endif()
if(TARGET PkgConfig::libavdevice)
    list(APPEND mltavformat_libs PkgConfig::libavdevice)
    list(APPEND mltavformat_defs AVDEVICE)
endif()
if(TARGET PkgConfig::libswresample)
    list(APPEND mltavformat_libs PkgConfig::libswresample)
    list(APPEND mltavformat_defs SWRESAMPLE)
    list(APPEND mltavformat_srcs
        filter_swresample.c)
endif()
add_library(mltavformat MODULE ${mltavformat_srcs})
target_link_libraries(mltavformat ${mltavformat_libs})
target_compile_definitions(mltavformat PRIVATE ${mltavformat_defs})
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltavformat PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltavformat LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
install(FILES blacklist.txt producer_avformat.yml consumer_avformat.yml yuv_only.txt resolution_scale.yml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/avformat)
