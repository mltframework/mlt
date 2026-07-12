file(GLOB YML "*.yml")
add_custom_target(Other_jackrack_Files SOURCES
  ${YML}
)

if(TARGET JACK::JACK)
  add_library(mltjackrack MODULE factory.c)

  include(GenerateExportHeader)
  generate_export_header(mltjackrack)
  target_compile_options(mltjackrack PRIVATE ${MLT_COMPILE_OPTIONS})

  target_sources(mltjackrack PRIVATE consumer_jack.c)
  target_include_directories(mltjackrack PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_compile_definitions(mltjackrack PRIVATE WITH_JACK)
  target_link_libraries(mltjackrack PRIVATE mlt Threads::Threads JACK::JACK)
  if(MSVC)
      target_link_libraries(mltjackrack PRIVATE PThreads4W::PThreads4W msvccompat)
  endif()
  install(FILES consumer_jack.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/jackrack)

  if(USE_LADSPA)
    target_sources(mltjackrack PRIVATE
      filter_jackrack.c
      jack_rack.c jack_rack.h
      lock_free_fifo.c lock_free_fifo.h
      plugin.c plugin.h
      plugin_desc.c plugin_desc.h
      plugin_mgr.c plugin_mgr.h
      plugin_settings.c plugin_settings.h
      process.c process.h
      producer_ladspa.c
      filter_ladspa.c
    )
    target_compile_definitions(mltjackrack PRIVATE WITH_LADSPA)
    if(RELOCATABLE)
      target_compile_definitions(mltjackrack PRIVATE RELOCATABLE)
    endif()
    target_link_libraries(mltjackrack PRIVATE ${CMAKE_DL_LIBS} PkgConfig::xml PkgConfig::glib)
    if(MSVC)
      target_link_libraries(mltjackrack PRIVATE dirent)
    else()
      target_link_libraries(mltjackrack PRIVATE m)
    endif()

    install(FILES filter_jack.yml filter_jackrack.yml blacklist.txt DESTINATION ${MLT_INSTALL_DATA_DIR}/jackrack)
  endif()

  set_target_properties(mltjackrack PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")
  install(TARGETS mltjackrack LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})
else()
  message(STATUS "JACK not found; disabling mltjackrack target")
endif()


if(USE_LADSPA)
  add_library(mltladspa MODULE factory.c)

  include(GenerateExportHeader)
  generate_export_header(mltladspa)
  target_compile_options(mltladspa PRIVATE ${MLT_COMPILE_OPTIONS})

  target_sources(mltladspa PRIVATE
    jack_rack.c jack_rack.h
    lock_free_fifo.c lock_free_fifo.h
    plugin.c plugin.h
    plugin_desc.c plugin_desc.h
    plugin_mgr.c plugin_mgr.h
    plugin_settings.c plugin_settings.h
    process.c process.h
    producer_ladspa.c
    filter_ladspa.c
  )
  target_include_directories(mltladspa PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_compile_definitions(mltladspa PRIVATE WITH_LADSPA)
  if(RELOCATABLE)
    target_compile_definitions(mltladspa PRIVATE RELOCATABLE)
  endif()
  target_link_libraries(mltladspa PRIVATE ${CMAKE_DL_LIBS} mlt PkgConfig::xml PkgConfig::glib Threads::Threads)
  if(MSVC)
    target_link_libraries(mltladspa PRIVATE dirent)
    target_link_libraries(mltladspa PRIVATE PThreads4W::PThreads4W)
  else()
    target_link_libraries(mltladspa PRIVATE m)
  endif()

  install(FILES filter_ladspa.yml producer_ladspa.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/jackrack)

  if(USE_LV2)
    target_compile_definitions(mltladspa PRIVATE WITH_LV2)
    target_link_libraries(mltladspa PRIVATE PkgConfig::lilv)
    target_sources(mltladspa PRIVATE
      filter_lv2.c producer_lv2.c
      lv2_context.c lv2_context.h
      lv2_plugin.c lv2_plugin.h
      lv2_process.c lv2_process.h
      lv2_plugin_settings.c lv2_plugin_settings.h
      lv2_urid_helper.h)
    install(FILES filter_lv2.yml producer_lv2.yml lv2blacklist.txt DESTINATION ${MLT_INSTALL_DATA_DIR}/jackrack)
  endif()

  if(USE_VST2)
    target_compile_definitions(mltladspa PRIVATE WITH_VST2)
    target_sources(mltladspa PRIVATE
      filter_vst2.c producer_vst2.c
      vst2_context.c vst2_context.h
      vst2_plugin.c vst2_plugin.h
      vst2_process.c vst2_process.h
      vst2_plugin_settings.c vst2_plugin_settings.h
      vestige.h)
    install(FILES filter_vst2.yml producer_vst2.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/jackrack)
  endif()

  set_target_properties(mltladspa PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")
  install(TARGETS mltladspa LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})
endif()
