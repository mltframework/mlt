include(UseSWIG)

if(SWIG_JAVA AND JNI_FOUND)
  add_subdirectory(java)
endif()

if(SWIG_LUA AND LUA_FOUND)
  add_subdirectory(lua)
endif()

if(SWIG_NODEJS AND NODEJS_INCLUDE_DIRS)
  add_subdirectory(nodejs)
endif()

if(SWIG_PERL AND PERLLIBS_FOUND)
  add_subdirectory(perl)
endif()

if(SWIG_PHP AND PHP_FOUND)
  add_subdirectory(php)
endif()

if(SWIG_PYTHON AND Python3_FOUND)
  add_subdirectory(python)
endif()

if(SWIG_RUBY AND RUBY_FOUND)
  add_subdirectory(ruby)
endif()

if(SWIG_TCL AND TCL_FOUND)
  add_subdirectory(tcl)
endif()
