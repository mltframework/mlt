file(GLOB mlt++_src *.cpp)
add_library(mlt++ SHARED ${mlt++_src})
set_target_properties(mlt++ PROPERTIES SOVERSION 3 VERSION ${MLT_VERSION} CXX_STANDARD 14)
if(WIN32)
    target_compile_definitions(mlt++ PUBLIC MLTPP_EXPORTS)
endif()
target_include_directories(mlt++ PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_link_libraries(mlt++ mlt)
if(WIN32)
    install(TARGETS mlt++ LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(TARGETS mlt++ ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
else()
    install(TARGETS mlt++ DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

file(GLOB mlt++_head *.h)
install(FILES ${mlt++_head} DESTINATION include/mlt++)

configure_file(mlt++.pc.in mlt++.pc @ONLY)
install (FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mlt++.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    COMPONENT Development)
