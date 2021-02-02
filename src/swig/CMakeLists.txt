include(UseSWIG)

if(SWIG_PYTHON AND Python3_FOUND)
  add_subdirectory(python)
endif()

if(SWIG_RUBY AND RUBY_FOUND)
  add_subdirectory(ruby)
endif()