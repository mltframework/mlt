add_library(mltndi MODULE
  consumer_ndi.c
  factory.c
  producer_ndi.c
)

target_link_libraries(mltndi PRIVATE mlt NDI::NDI)

set_target_properties(mltndi PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltndi LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
install(FILES consumer_ndi.yml producer_ndi.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/ndi)
