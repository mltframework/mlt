add_library(mltmotion_est MODULE
  arrow_code.c
  factory.c
  filter_autotrack_rectangle.c
  filter_crop_detect.c
  filter_motion_est.c
  filter_vismv.c
  producer_slowmotion.c
)

target_link_libraries(mltmotion_est PRIVATE mlt m)

set_target_properties(mltmotion_est PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltmotion_est LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES
  filter_autotrack_rectangle.yml
  filter_motion_est.yml
  filter_vismv.yml
  producer_slowmotion.yml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/motion_est
)
