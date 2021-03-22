add_library(mltjack MODULE factory.c)

target_compile_options(mltjack PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltjack PRIVATE mlt Threads::Threads)

if(TARGET PkgConfig::jack)
  target_sources(mltjack PRIVATE consumer_jack.c)
  target_link_libraries(mltjack PRIVATE PkgConfig::jack)
  target_compile_definitions(mltjack PRIVATE WITH_JACK)
  install(FILES consumer_jack.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/jackrack)
endif()

if(GPL AND TARGET PkgConfig::xml AND TARGET PkgConfig::glib AND ladspa_h_FOUND)
  target_sources(mltjack PRIVATE
    jack_rack.c
    lock_free_fifo.c
    plugin.c
    plugin_desc.c
    plugin_mgr.c
    plugin_settings.c
    process.c
    producer_ladspa.c
    filter_ladspa.c
  )
  target_link_libraries(mltjack PRIVATE ${CMAKE_DL_LIBS} m PkgConfig::xml PkgConfig::glib)
  target_compile_definitions(mltjack PRIVATE GPL)
  install(FILES filter_jack.yml filter_ladspa.yml producer_ladspa.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/jackrack)

  if(TARGET PkgConfig::jack)
    target_sources(mltjack PRIVATE filter_jackrack.c)
    install(FILES filter_jackrack.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/jackrack)
  endif()
endif()

set_target_properties(mltjack PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltjack LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})
