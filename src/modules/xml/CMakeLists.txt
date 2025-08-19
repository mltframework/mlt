add_library(mltxml MODULE
  common.c common.h
  consumer_xml.c
  factory.c
  producer_xml.c
  producer_xml-clip.c
)

file(GLOB YML "*.yml")
add_custom_target(Other_xml_Files SOURCES
  ${YML}
  mlt-xml.dtd
)

target_compile_options(mltxml PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltxml PRIVATE mlt Threads::Threads PkgConfig::xml)
if(MSVC)
  target_link_libraries(mltxml PRIVATE PThreads4W::PThreads4W)
  # Explicitly include vcpkg pthread headers for MSVC
  if(DEFINED VCPKG_TARGET_TRIPLET)
    target_include_directories(mltxml PRIVATE "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")
  endif()
else()
  target_link_libraries(mltxml PRIVATE m)
endif()

set_target_properties(mltxml PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltxml LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  consumer_xml.yml
  producer_xml-nogl.yml
  producer_xml-string.yml
  producer_xml.yml
  producer_xml-clip.yml
  mlt-xml.dtd
  DESTINATION ${MLT_INSTALL_DATA_DIR}/xml
)
