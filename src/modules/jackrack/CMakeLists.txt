add_library(mltjackrack MODULE factory.c)

target_compile_options(mltjackrack PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltjackrack PRIVATE mlt Threads::Threads)

if(TARGET JACK::JACK)
  target_sources(mltjackrack PRIVATE consumer_jack.c)
  target_link_libraries(mltjackrack PRIVATE JACK::JACK)
  target_compile_definitions(mltjackrack PRIVATE WITH_JACK)
  install(FILES consumer_jack.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/jackrack)
endif()

if(GPL AND TARGET PkgConfig::xml AND TARGET PkgConfig::glib AND ladspa_h_FOUND)
  target_sources(mltjackrack PRIVATE
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
  target_link_libraries(mltjackrack PRIVATE ${CMAKE_DL_LIBS} m PkgConfig::xml PkgConfig::glib)
  target_compile_definitions(mltjackrack PRIVATE GPL)
  if(RELOCATABLE)
    target_compile_definitions(mltjackrack PRIVATE RELOCATABLE)
  endif()

  install(FILES filter_jack.yml filter_ladspa.yml producer_ladspa.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/jackrack)

  if(TARGET JACK::JACK)
    target_sources(mltjackrack PRIVATE filter_jackrack.c)
    install(FILES filter_jackrack.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/jackrack)
  endif()
endif()

set_target_properties(mltjackrack PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltjackrack LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})
