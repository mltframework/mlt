include(UseSWIG)

if(SWIG_PYTHON AND Python3_FOUND)
  add_subdirectory(python)
endif()
