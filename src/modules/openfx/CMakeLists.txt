add_library(mltopenfx MODULE
  factory.c
  mlt_openfx.c mlt_openfx.h
  filter_openfx.c
)

add_custom_target(Other_openfx_Files SOURCES
  filter_openfx.yml
)

if(GPL AND TARGET PkgConfig::glib)
  include(GenerateExportHeader)
  generate_export_header(mltopenfx)
  target_compile_options(mltopenfx PRIVATE ${MLT_COMPILE_OPTIONS})
  target_include_directories(mltopenfx PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(mltopenfx PRIVATE mlt PkgConfig::glib ${CMAKE_DL_LIBS})
  target_include_directories(mltopenfx PRIVATE openfx/include)
  if(NOT MSVC)
    target_link_libraries(mltopenfx PRIVATE m)
  endif()
endif()

set_target_properties(mltopenfx PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltopenfx LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_openfx.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/openfx)
