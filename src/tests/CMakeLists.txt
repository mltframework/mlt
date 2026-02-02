set(CMAKE_AUTOMOC ON)

function(add_qt_test)
  set(options "")
  set(oneValueArgs TEST_NAME)
  set(multiValueArgs LINK_LIBRARIES INCLUDE_DIRS SOURCE_FILES)
  cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(_testname test_${arg_TEST_NAME})

  add_executable(${_testname} ${_testname}/${_testname}.cpp ${arg_SOURCE_FILES})

  target_compile_options(${_testname} PRIVATE ${MLT_COMPILE_OPTIONS})

  target_link_libraries(${_testname} PRIVATE
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Test
    mlt++
    ${ARG_LINK_LIBRARIES}
  )
  if(MSVC)
    target_link_libraries(${_testname} PRIVATE PThreads4W::PThreads4W)
  endif()

  target_include_directories(${_testname} PRIVATE ${arg_INCLUDE_DIRS})
  add_test(NAME "QtTest:${arg_TEST_NAME}" COMMAND ${_testname})

  if(NOT WIN32)
    set_tests_properties("QtTest:${arg_TEST_NAME}" PROPERTIES ENVIRONMENT "LANG=en_US")
  endif()

endfunction()

add_qt_test(TEST_NAME animation)
add_qt_test(TEST_NAME audio)
add_qt_test(TEST_NAME events)
add_qt_test(TEST_NAME filter)
add_qt_test(TEST_NAME frame)
add_qt_test(TEST_NAME image)
add_qt_test(TEST_NAME multitrack)
add_qt_test(TEST_NAME playlist)
add_qt_test(TEST_NAME producer)
add_qt_test(TEST_NAME properties)
add_qt_test(TEST_NAME repository)
add_qt_test(TEST_NAME service)
add_qt_test(TEST_NAME tractor)
add_qt_test(TEST_NAME xml)

if(MOD_AVFORMAT)
  add_qt_test(TEST_NAME mod_avformat)
endif()

if(MOD_QT6)
  add_qt_test(
    TEST_NAME mod_qt_gps
    SOURCE_FILES
      "${CMAKE_SOURCE_DIR}/src/modules/qt/gps_parser.h"
      "${CMAKE_SOURCE_DIR}/src/modules/qt/gps_parser.cpp"
  )
endif()

if(Kwalify_FOUND)
  file(GLOB YML_FILES "${CMAKE_SOURCE_DIR}/src/modules/*/*.yml")
  foreach(YML_FILE ${YML_FILES})
    get_filename_component(FILE_NAME ${YML_FILE} NAME)
    file(RELATIVE_PATH KWALIFY_TEST_NAME "${CMAKE_SOURCE_DIR}/src/modules" ${YML_FILE})
    if(NOT FILE_NAME MATCHES "resolution_scale.yml")
      add_test(NAME "kwalify:${KWALIFY_TEST_NAME}" COMMAND ${Kwalify_EXECUTABLE} -f "${CMAKE_SOURCE_DIR}/src/framework/metaschema.yaml" ${YML_FILE})
    endif()
  endforeach()
endif()
