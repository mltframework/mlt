add_library(mltrtaudio MODULE
  consumer_rtaudio.cpp
)

file(GLOB YML "*.yml")
add_custom_target(Other_rtaudio_Files SOURCES
  ${YML}
)
include(GenerateExportHeader)
generate_export_header(mltrtaudio)
target_compile_options(mltrtaudio PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltrtaudio PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltrtaudio PRIVATE mlt Threads::Threads)

if(MSVC)
  target_link_libraries(mltrtaudio PRIVATE PThreads4W::PThreads4W)
endif()

if(TARGET PkgConfig::rtaudio)
  target_link_libraries(mltrtaudio PRIVATE PkgConfig::rtaudio)
else()
  target_sources(mltrtaudio PRIVATE RtAudio.cpp RtAudio.h)
  target_include_directories(mltrtaudio PRIVATE .)
  if(APPLE)
    target_link_libraries(mltrtaudio PRIVATE "-framework CoreAudio" "-framework CoreFoundation")
    target_compile_definitions(mltrtaudio PRIVATE __MACOSX_CORE__)
  elseif(WIN32)
    target_link_libraries(mltrtaudio PRIVATE ole32 dsound winmm ksuser mfplat mfuuid wmcodecdspuuid)
    target_compile_definitions(mltrtaudio PRIVATE __WINDOWS_DS__ __WINDOWS_WASAPI__)
  else()
    if(TARGET PkgConfig::alsa)
      target_link_libraries(mltrtaudio PRIVATE PkgConfig::alsa)
      target_compile_definitions(mltrtaudio PRIVATE __LINUX_ALSA__)
    endif()
    if(TARGET PkgConfig::libpulse-simple)
      target_link_libraries(mltrtaudio PRIVATE PkgConfig::libpulse-simple)
      target_compile_definitions(mltrtaudio PRIVATE __LINUX_PULSE__)
    endif()
    if(NOT (TARGET PkgConfig::alsa OR TARGET PkgConfig::libpulse-simple))
      target_link_libraries(mltrtaudio PRIVATE ossaudio)
      target_compile_definitions(mltrtaudio PRIVATE __LINUX_OSS__)
    endif()
  endif()
endif()

set_target_properties(mltrtaudio PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltrtaudio LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES consumer_rtaudio.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/rtaudio)
