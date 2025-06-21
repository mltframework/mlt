add_library(mltrtaudio MODULE
  consumer_rtaudio.cpp
)

file(GLOB YML "*.yml")
add_custom_target(Other_rtaudio_Files SOURCES
  ${YML}
)

target_compile_options(mltrtaudio PRIVATE ${MLT_COMPILE_OPTIONS})

if (MSVC)
  target_link_libraries(mltrtaudio PRIVATE mlt PThreads4W::PThreads4W)

  find_library(GETTIMEOFDAY_LIBRARY
          NAMES gettimeofday
          PATHS "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/debug/lib/manual-link"
          NO_DEFAULT_PATH
  )

  message(STATUS ">> Linking with gettimeofday = ${GETTIMEOFDAY_LIBRARY}")

  if (NOT GETTIMEOFDAY_LIBRARY)
    message(FATAL_ERROR "Could not find debug gettimeofday.lib")
  endif()

  target_link_libraries(mltrtaudio PRIVATE ${GETTIMEOFDAY_LIBRARY})
else ()
  target_link_libraries(mltrtaudio PRIVATE mlt Threads::Threads)
endif ()


if(TARGET PkgConfig::rtaudio)
  if(MSVC)
    # --- 修复 rtaudio 传递的错误 pthread 依赖 ---
    if(TARGET PkgConfig::rtaudio) # 假设 vcpkg 创建的 target 名是 rtaudio::rtaudio
      # 获取它传递的链接库
      get_target_property(RTAUDIO_LIBS PkgConfig::rtaudio INTERFACE_LINK_LIBRARIES)
      # 从列表中移除 'pthread'
      list(REMOVE_ITEM RTAUDIO_LIBS "pthread")
      # 将修改后的、干净的列表设置回去
      set_target_properties(PkgConfig::rtaudio PROPERTIES INTERFACE_LINK_LIBRARIES "${RTAUDIO_LIBS}")
    endif()
  endif()
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
