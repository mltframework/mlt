add_library(mltopencv MODULE factory.c filter_opencv_tracker.cpp)
target_link_libraries(mltopencv mlt ${OpenCV_LIBS})

set_target_properties(mltopencv PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltopencv LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
install(FILES filter_opencv_tracker.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/opencv)
