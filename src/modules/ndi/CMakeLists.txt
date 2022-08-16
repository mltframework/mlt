add_library(mltndi MODULE
  consumer_ndi.c
  factory.c factory.h
  producer_ndi.c
)

file(GLOB YML "*.yml")
add_custom_target(Other_ndi_Files SOURCES
  ${YML}
)

target_compile_options(mltndi PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltndi PRIVATE mlt NDI::NDI)

if(CPU_SSE)
  target_compile_definitions(mltndi PRIVATE USE_SSE)
endif()

set_target_properties(mltndi PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltndi LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})
install(FILES consumer_ndi.yml producer_ndi.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/ndi)
