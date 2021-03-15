add_library(mltmovit MODULE
  consumer_xgl.c
  factory.c
  filter_glsl_manager.cpp
  filter_movit_blur.cpp
  filter_movit_convert.cpp
  filter_movit_crop.cpp
  filter_movit_deconvolution_sharpen.cpp
  filter_movit_diffusion.cpp
  filter_movit_flip.cpp
  filter_movit_glow.cpp
  filter_movit_lift_gamma_gain.cpp
  filter_movit_mirror.cpp
  filter_movit_opacity.cpp
  filter_movit_rect.cpp
  filter_movit_resample.cpp
  filter_movit_resize.cpp
  filter_movit_saturation.cpp
  filter_movit_vignette.cpp
  filter_movit_white_balance.cpp
  mlt_movit_input.cpp
  transition_movit_luma.cpp
  transition_movit_mix.cpp
  transition_movit_overlay.cpp
)

target_compile_options(mltmovit PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltmovit PRIVATE m Threads::Threads mlt mlt++ OpenGL::GL PkgConfig::movit)

if(UNIX AND NOT APPLE)
  target_link_libraries(mltmovit PRIVATE X11::X11)
endif()

pkg_get_variable(SHADERDIR movit shaderdir)
target_compile_definitions(mltmovit PRIVATE SHADERDIR="${SHADERDIR}")

set_target_properties(mltmovit PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltmovit LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  filter_movit_blur.yml
  filter_movit_deconvolution_sharpen.yml
  filter_movit_diffusion.yml
  filter_movit_flip.yml
  filter_movit_glow.yml
  filter_movit_lift_gamma_gain.yml
  filter_movit_mirror.yml
  filter_movit_opacity.yml
  filter_movit_rect.yml
  filter_movit_saturation.yml
  filter_movit_vignette.yml
  filter_movit_white_balance.yml
  transition_movit_luma.yml
  transition_movit_mix.yml
  transition_movit_overlay.yml
  DESTINATION ${MLT_INSTALL_DATA_DIR}/opengl
)
