add_library(mltxine MODULE
  common.c common.h
  factory.c
  deinterlace.c deinterlace.h
  yadif.c yadif.h
  filter_deinterlace.c
  link_deinterlace.c
)

file(GLOB YML "*.yml")
add_custom_target(Other_xine_Files SOURCES
  ${YML}
)
include(GenerateExportHeader)
generate_export_header(mltxine)
target_compile_options(mltxine PRIVATE ${MLT_COMPILE_OPTIONS})
target_include_directories(mltxine PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mltxine PRIVATE mlt ${CMAKE_DL_LIBS})

set_target_properties(mltxine PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(mltxine PRIVATE PIC)
if(MSVC)
  target_link_libraries(mltxine PRIVATE msvccompat)
endif()


if(CPU_MMX)
  target_compile_definitions(mltxine PRIVATE USE_MMX)
  target_sources(mltxine PRIVATE cpu_accel.c)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    # avoid crash in yadif filter_line_sse2
    target_compile_options(mltxine PRIVATE -fno-tree-dominator-opts -fno-tree-pre)
  endif()
endif()

if(CPU_SSE)
  target_compile_definitions(mltxine PRIVATE USE_SSE)
endif()

if(CPU_SSE2)
  target_compile_definitions(mltxine PRIVATE USE_SSE2)
endif()

if(CPU_X86_32)
  target_compile_definitions(mltxine PRIVATE ARCH_X86)
endif()

if(CPU_X86_64)
  target_compile_definitions(mltxine PRIVATE ARCH_X86_64)
endif()

set_target_properties(mltxine PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltxine LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  filter_deinterlace.yml
  link_deinterlace.yml
  DESTINATION ${MLT_INSTALL_DATA_DIR}/xine
)
