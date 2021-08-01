add_library(mltjackrack MODULE consumer_jack.c factory.c)
target_link_libraries(mltjackrack PRIVATE mlt Threads::Threads)

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
  if(APPLE AND RELOCATABLE)
    target_compile_definitions(mltjackrack PRIVATE RELOCATABLE)
  endif()
  install(FILES
    filter_jack.yml
    filter_ladspa.yml
    producer_ladspa.yml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/jackrack
  )
  if(TARGET JACK::JACK)
    target_sources(mltjackrack PRIVATE filter_jackrack.c)
    target_link_libraries(mltjackrack PRIVATE JACK::JACK)
    install(FILES filter_jackrack.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/jackrack)
  endif()
endif()

# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltjackrack PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)

install(TARGETS mltjackrack LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES consumer_jack.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/jackrack)
