find_package(SWIG)
if (${SWIG_Found})
    include(UseSWIG)

    set_property(SOURCE mlt.i PROPERTY USE_TARGET_INCLUDE_DIRECTORIES TRUE)
    set_property(SOURCE mlt.i PROPERTY CPLUSPLUS ON)

    find_package(Python3 COMPONENTS Interpreter Development)

    if(Python3_FOUND)
      swig_add_library(pymlt LANGUAGE python SOURCES mlt.i)
      target_include_directories(pymlt PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/mlt++ ${Python3_INCLUDE_DIRS})
      target_link_libraries(pymlt mlt mlt++)
      set_target_properties(pymlt PROPERTIES PREFIX "_" OUTPUT_NAME "mlt")

      if(NOT DEFINED PYTHON_MODULE_PATH)
        if(EXISTS "/etc/debian_version")
          set(PYTHON_MODULE_PATH "lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/dist-packages")
        else()
          set(PYTHON_MODULE_PATH "lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages")
        endif()
      endif()

      install(TARGETS pymlt DESTINATION ${PYTHON_MODULE_PATH})
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mlt.py DESTINATION ${PYTHON_MODULE_PATH})
    endif()
endif()
