file(GLOB mltxml_src *.c)
add_library(mltxml MODULE ${mltxml_src})
target_link_libraries(mltxml mlt Threads::Threads PkgConfig::xml)
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltxml PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltxml LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml} mlt-xml.dtd DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/xml)
