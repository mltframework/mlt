add_library(mltxml MODULE
  common.c common.h
  consumer_xml.c
  factory.c
  producer_xml.c
)

file(GLOB YML "*.yml")
add_custom_target(Other_xml_Files SOURCES
  ${YML}
  mlt-xml.dtd
)

target_compile_options(mltxml PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltxml PRIVATE mlt m Threads::Threads PkgConfig::xml)

set_target_properties(mltxml PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltxml LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  consumer_xml.yml
  producer_xml-nogl.yml
  producer_xml-string.yml
  producer_xml.yml
  mlt-xml.dtd
  DESTINATION ${MLT_INSTALL_DATA_DIR}/xml
)
