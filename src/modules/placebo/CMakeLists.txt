find_package(PkgConfig REQUIRED)
pkg_check_modules(libplacebo IMPORTED_TARGET libplacebo>=5.229)
if(NOT libplacebo_FOUND)
  return()
endif()

add_library(mltplacebo MODULE
  color_space.c color_space.h
  convert.c convert.h
  factory.c
  filter_placebo_blend_mode.c
  filter_placebo_color_transform.c
  filter_placebo_render.c
  filter_placebo_scale.c
  filter_placebo_shader.c
  transition_placebo_blend.c
  transition_placebo_luma_mix.c
  transition_common.c transition_common.h
  gpu_context.c gpu_context.h
  vulkan_frame.c vulkan_frame.h
)

file(GLOB YML "*.yml")
add_custom_target(Other_placebo_Files SOURCES
  ${YML}
)

include(GenerateExportHeader)
generate_export_header(mltplacebo)
target_compile_options(mltplacebo PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltplacebo PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltplacebo PRIVATE mlt Threads::Threads PkgConfig::libplacebo)

# Only link D3D11 if libplacebo was built with D3D11 support
include(CheckCSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${libplacebo_INCLUDE_DIRS})
check_c_source_compiles("
  #include <libplacebo/config.h>
  #if !defined(PL_HAVE_D3D11) || !PL_HAVE_D3D11
  #error no d3d11
  #endif
  int main(void) { return 0; }
" PLACEBO_HAS_D3D11)
if(PLACEBO_HAS_D3D11)
  target_link_libraries(mltplacebo PRIVATE d3d11 dxgi)
endif()
if(WIN32)
  target_link_libraries(mltplacebo PRIVATE shell32 ole32)
endif()

if(MSVC)
  target_link_libraries(mltplacebo PRIVATE PThreads4W::PThreads4W)
endif()

if(SDL2_FOUND)
  target_sources(mltplacebo PRIVATE consumer_placebo_sdl.c)
  target_compile_definitions(mltplacebo PRIVATE PLACEBO_SDL)
  target_link_libraries(mltplacebo PRIVATE SDL2::SDL2)
endif()

set_target_properties(mltplacebo PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltplacebo LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

set(PLACEBO_YML
  filter_placebo_blend_mode.yml
  filter_placebo_color_transform.yml
  filter_placebo_render.yml
  filter_placebo_scale.yml
  filter_placebo_shader.yml
  transition_placebo_blend.yml
  transition_placebo_luma_mix.yml
)
if(SDL2_FOUND)
  list(APPEND PLACEBO_YML consumer_placebo_sdl.yml)
endif()
install(FILES ${PLACEBO_YML} DESTINATION ${MLT_INSTALL_DATA_DIR}/placebo)
install(DIRECTORY shaders/ DESTINATION ${MLT_INSTALL_DATA_DIR}/placebo/shaders)
