foreach(MLT_TEST_NAME animation audio events filter frame playlist properties repository service tractor)
  add_executable(test_${MLT_TEST_NAME} test_${MLT_TEST_NAME}/test_${MLT_TEST_NAME}.cpp)
  target_link_libraries(test_${MLT_TEST_NAME} PRIVATE Qt5::Core Qt5::Test mlt++)
  add_test(NAME ${MLT_TEST_NAME} COMMAND test_${MLT_TEST_NAME})
  set_tests_properties(${MLT_TEST_NAME} PROPERTIES
    ENVIRONMENT "LANG=en_US;MLT_REPOSITORY=${CMAKE_BINARY_DIR}/src/modules;MLT_DATA=${CMAKE_SOURCE_DIR}/src/modules;MLT_PROFILES_PATH=${CMAKE_SOURCE_DIR}/profiles;MLT_PRESETS_PATH=${CMAKE_SOURCE_DIR}/presets")
endforeach()
