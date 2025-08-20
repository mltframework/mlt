add_library(mltrubberband MODULE factory.c filter_rbpitch.cpp)

file(GLOB YML "*.yml")
add_custom_target(Other_rubberband_Files SOURCES
  ${YML}
)

target_compile_options(mltrubberband PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltrubberband PRIVATE mlt PkgConfig::rubberband)

if(MSVC)
  target_link_libraries(mltrubberband PRIVATE msvccompat PThreads4W::PThreads4W)
  mlt_add_vcpkg_include_dirs(mltrubberband)
endif()

set_target_properties(mltrubberband PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltrubberband LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_rbpitch.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/rubberband)
