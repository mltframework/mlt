find_program(Kwalify_EXECUTABLE kwalify)

if(Kwalify_EXECUTABLE)
  execute_process(COMMAND ${Kwalify_EXECUTABLE} -v
    OUTPUT_VARIABLE Kwalify_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Kwalify
  FOUND_VAR Kwalify_FOUND
  REQUIRED_VARS Kwalify_EXECUTABLE
  VERSION_VAR Kwalify_VERSION
)
