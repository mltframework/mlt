add_subdirectory(framework)
add_subdirectory(melt)
add_subdirectory(mlt++)
add_subdirectory(modules)

if(SWIG_FOUND)
  add_subdirectory(swig)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
