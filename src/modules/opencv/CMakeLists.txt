add_library(mltopencv MODULE factory.c filter_opencv_tracker.cpp)

target_compile_options(mltopencv PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltopencv PRIVATE mlt ${OpenCV_LIBS})

set_target_properties(mltopencv PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltopencv LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})
install(FILES filter_opencv_tracker.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/opencv)
