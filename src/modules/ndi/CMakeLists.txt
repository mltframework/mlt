add_library(mltndi MODULE
  consumer_ndi.c
  factory.c
  producer_ndi.c
)

target_compile_options(mltndi PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltndi PRIVATE mlt NDI::NDI)

if(CPU_SSE)
  target_compile_definitions(mltndi PRIVATE USE_SSE)
endif()

set_target_properties(mltndi PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltndi LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
install(FILES consumer_ndi.yml producer_ndi.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/ndi)
