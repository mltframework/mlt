set(mltgtk2_src factory.c)
set(mltgtk2_lib mlt m Threads::Threads)
set(mltgtk2_def "")
if(TARGET GTK2::gtk)
    list(APPEND mltgtk2_src consumer_gtk2.c)
    list(APPEND mltgtk2_lib GTK2::gtk GTK2::gdk_pixbuf GTK2::pango PkgConfig::fontconfig)
    list(APPEND mltgtk2_def USE_GTK2)
endif()
# Only for MMX but not x86_64: deprecated
# list(APPEND mltgtk2_src have_mmx.S scale_line_22_yuv_mmx.S)
add_library(mltgtk2 MODULE ${mltgtk2_src})
target_link_libraries(mltgtk2 ${mltgtk2_lib})
target_compile_definitions(mltgtk2 PRIVATE ${mltgtk2_def})
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltgtk2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltgtk2 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/gtk2)
