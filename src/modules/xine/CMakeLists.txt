set(mltxine_src factory.c
  deinterlace.c
  yadif.c
  filter_deinterlace.c)
if(X86_64)
  list(APPEND mltxine_src cpu_accel.c)
  if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    # avoid crash in yadif filter_line_sse2
    add_compile_options(-fno-tree-dominator-opts -fno-tree-pre)
  endif()
endif()
add_library(mltxine MODULE ${mltxine_src})
target_link_libraries(mltxine mlt)
target_compile_definitions(mltxine PRIVATE PIC)
set_property(TARGET mltxine PROPERTY POSITION_INDEPENDENT_CODE ON)
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltxine PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltxine LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/xine)
