file(GLOB mltcore_src *.c)
set(mltcore_inc "")
if(WIN32)
    list(APPEND mltcore_src ${CMAKE_SOURCE_DIR}/src/win32/fnmatch.c)
    list(APPEND mltcore_inc ${CMAKE_SOURCE_DIR}/src/win32)
endif()

if(NOT X86_64)
    list(REMOVE_ITEM mltcore_src ${CMAKE_CURRENT_SOURCE_DIR}/composite_line_yuv_sse2_simple.c)
endif()

add_library(mltcore MODULE ${mltcore_src})
target_include_directories(mltcore PRIVATE ${mltcore_inc})
target_link_libraries(mltcore mlt Threads::Threads)
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltcore PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltcore LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES data_fx.properties loader.dict loader.ini ${yml}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/core)
