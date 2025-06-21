add_library(mltgdk MODULE
  factory.c
  filter_rescale.c
  pixops.c
  producer_pixbuf.c
)
if(MSVC)
  target_link_options(mltgdk PRIVATE "/NODEFAULTLIB:m.lib")
endif()

file(GLOB YML "*.yml")
add_custom_target(Other_gdk_Files SOURCES
  ${YML}
)

target_compile_options(mltgdk PRIVATE ${MLT_COMPILE_OPTIONS})
# =================== MSVC 链接修复程序 ===================
if(MSVC)
  # --- 修复 PkgConfig::GdkPixbuf ---
  if(TARGET PkgConfig::GdkPixbuf)
    # 获取它传递的链接库
    get_target_property(GDK_LIBS PkgConfig::GdkPixbuf INTERFACE_LINK_LIBRARIES)
    # 从列表中移除 'm'
    list(REMOVE_ITEM GDK_LIBS "m")
    # 将修改后的、干净的列表设置回去
    set_target_properties(PkgConfig::GdkPixbuf PROPERTIES INTERFACE_LINK_LIBRARIES "${GDK_LIBS}")
  endif()

  # --- 修复 PkgConfig::libexif ---
  if(TARGET PkgConfig::libexif)
    get_target_property(EXIF_LIBS PkgConfig::libexif INTERFACE_LINK_LIBRARIES)
    list(REMOVE_ITEM EXIF_LIBS "m")
    set_target_properties(PkgConfig::libexif PROPERTIES INTERFACE_LINK_LIBRARIES "${EXIF_LIBS}")
  endif()

  # 对其他所有通过 pkg_check_modules 找到的目标也进行同样的操作
  # 例如 Pango, fftw3, ebur128 等
endif()
# ==========================================================
target_link_libraries(mltgdk PRIVATE mlt Threads::Threads PkgConfig::GdkPixbuf)
if(NOT MSVC)
  target_link_libraries(mltgdk PRIVATE m)
endif()

target_compile_definitions(mltgdk PRIVATE USE_PIXBUF)

if(TARGET PkgConfig::libexif)
  target_link_libraries(mltgdk PRIVATE PkgConfig::libexif)
  target_compile_definitions(mltgdk PRIVATE USE_EXIF)
endif()

if(TARGET PkgConfig::pango AND TARGET PkgConfig::fontconfig)
  target_sources(mltgdk PRIVATE producer_pango.c)
  target_link_libraries(mltgdk PRIVATE PkgConfig::pango PkgConfig::fontconfig PkgConfig::pangoft2)
  if(APPLE OR MINGW)
    target_link_libraries(mltgdk PRIVATE iconv)
  endif()
  target_compile_definitions(mltgdk PRIVATE USE_PANGO)
  install(FILES producer_pango.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/gdk)
endif()

if(CPU_MMX)
  target_sources(mltgdk PRIVATE have_mmx.S scale_line_22_yuv_mmx.S)
  target_compile_definitions(mltgdk PRIVATE USE_MMX)
endif()

if(CPU_X86_64)
  target_compile_definitions(mltgdk PRIVATE ARCH_X86_64)
endif()

set_target_properties(mltgdk PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltgdk LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_rescale.yml producer_pixbuf.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/gdk)
