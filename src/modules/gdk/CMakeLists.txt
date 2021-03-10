add_library(mltgdk MODULE
  factory.c
  filter_rescale.c
  pixops.c
  producer_pixbuf.c
)

target_compile_options(mltgdk PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltgdk PRIVATE mlt m Threads::Threads PkgConfig::GdkPixbuf)

target_compile_definitions(mltgdk PRIVATE USE_PIXBUF)

if(TARGET PkgConfig::libexif)
  target_link_libraries(mltgdk PRIVATE PkgConfig::libexif)
  target_compile_definitions(mltgdk PRIVATE USE_EXIF)
endif()

if(TARGET PkgConfig::pango AND TARGET PkgConfig::fontconfig)
  target_sources(mltgdk PRIVATE producer_pango.c)
  target_link_libraries(mltgdk PRIVATE PkgConfig::pango PkgConfig::fontconfig PkgConfig::pangoft2)
  target_compile_definitions(mltgdk PRIVATE USE_PANGO)
  install(FILES producer_pango.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/gdk)
endif()

# Only for MMX but not x86_64: deprecated
# target_sources(mltgdk PRIVATE have_mmx.S scale_line_22_yuv_mmx.S)

set_target_properties(mltgdk PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltgdk LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES filter_rescale.yml producer_pixbuf.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/gdk)
