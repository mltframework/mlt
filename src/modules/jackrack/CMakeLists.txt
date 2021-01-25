set(mltjack_src factory.c consumer_jack.c)
set(mltjack_lib mlt Threads::Threads PkgConfig::jack)

if(GPL AND TARGET PkgConfig::xml AND TARGET PkgConfig::glib AND ladspa_h_FOUND)
  set(mltjack_src ${mltjack_src}
    jack_rack.c
    lock_free_fifo.c
    plugin.c
    plugin_desc.c
    plugin_mgr.c
    plugin_settings.c
    process.c
    producer_ladspa.c
    filter_jackrack.c
    filter_ladspa.c)
  set(mltjack_lib ${mltjack_lib} ${CMAKE_DL_LIBS} m PkgConfig::xml PkgConfig::glib)
  set(mltjack_def GPL)
  file(GLOB yml *.yml blacklist.txt)
else()
  message(JACK NOGO)
  set(yml consumer_jack.yml)
endif()
add_library(mltjack MODULE ${mltjack_src})
target_link_libraries(mltjack ${mltjack_lib})
target_compile_definitions(mltjack PRIVATE ${mltjack_def})
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltjack PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltjack LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
install(FILES ${yml} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/jackrack)
