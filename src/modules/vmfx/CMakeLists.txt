add_library(mltvmfx MODULE
  factory.c
  filter_chroma_hold.c
  filter_chroma.c
  filter_mono.c
  filter_shape.c
  producer_pgm.c
)

target_link_libraries(mltvmfx mlt)

# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltvmfx PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)

install(TARGETS mltvmfx LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES
  filter_chroma_hold.yml
  filter_chroma.yml
  filter_mono.yml
  filter_shape.yml
  producer_pgm.yml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/vmfx
)
