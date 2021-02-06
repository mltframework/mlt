add_library(mltgtk2 MODULE factory.c)
target_link_libraries(mltgtk2 PRIVATE mlt m Threads::Threads)

if(TARGET GTK2::gtk AND TARGET PkgConfig::fontconfig AND TARGET PkgConfig::harfbuzz)
  target_sources(mltgtk2 PRIVATE consumer_gtk2.c)
  target_link_libraries(mltgtk2 PRIVATE
    GTK2::gtk
    GTK2::gdk_pixbuf
    GTK2::pango
    PkgConfig::fontconfig
    PkgConfig::harfbuzz
  )
  target_compile_definitions(mltgtk2 PRIVATE USE_GTK2)
  install(FILES consumer_gtk2_preview.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/gtk2)
endif()

# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltgtk2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)

install(TARGETS mltgtk2 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
