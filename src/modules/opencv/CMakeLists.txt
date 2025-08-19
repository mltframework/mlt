add_library(mltopencv MODULE factory.c filter_opencv_tracker.cpp)

file(GLOB YML "*.yml")
add_custom_target(Other_opencv_Files SOURCES
  ${YML}
)

target_compile_options(mltopencv PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltopencv PRIVATE mlt ${OpenCV_LIBS})

if(MSVC)
  target_link_libraries(mltopencv PRIVATE PThreads4W::PThreads4W)
  # Explicitly include vcpkg pthread headers for MSVC
  if(DEFINED VCPKG_TARGET_TRIPLET)
    target_include_directories(mltopencv PRIVATE "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")
  endif()
endif()

set_target_properties(mltopencv PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltopencv LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})
install(FILES filter_opencv_tracker.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/opencv)
