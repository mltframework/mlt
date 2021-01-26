file(GLOB mlt_src *.c)
if(WIN32)
    list(APPEND mlt_src ../win32/win32.c ../win32/strptime.c)
endif()
add_library(mlt SHARED ${mlt_src})
set_target_properties (mlt PROPERTIES SOVERSION ${MLT_VERSION_MAJOR} VERSION ${MLT_VERSION})
if(WIN32)
    target_link_libraries(mlt Iconv::Iconv)
    if(NODEPLOY)
        target_compile_definitions(mlt PRIVATE NODEPLOY)
    endif()
endif()
target_link_libraries(mlt ${CMAKE_DL_LIBS})
if(NOT (WIN32 OR APPLE))
    target_compile_definitions(mlt PRIVATE
        PREFIX_DATA="${CMAKE_INSTALL_FULL_DATADIR}/mlt"
        PREFIX_LIB="${CMAKE_INSTALL_FULL_LIBDIR}/mlt")
endif()

target_link_libraries(mlt m Threads::Threads)
target_include_directories(mlt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(mlt PUBLIC  ${CMAKE_SOURCE_DIR}/src)

if(WIN32)
    install(TARGETS mlt LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(TARGETS mlt ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
else()
    install(TARGETS mlt DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

file(GLOB mlt_head *.h)
install(FILES ${mlt_head} DESTINATION include/mlt/framework)

install(FILES metaschema.yaml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt)

configure_file(mlt-framework.pc.in mlt-framework.pc @ONLY)
install (FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mlt-framework.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    COMPONENT Development)
#configure_file(MLTConfig.cmake.in MLTConfig.cmake @ONLY)
#configure_file(MLTConfigVersion.cmake.in MLTConfigVersion.cmake @ONLY)
#install (FILES
#    ${CMAKE_CURRENT_BINARY_DIR}/MLTConfig.cmake
#    ${CMAKE_CURRENT_BINARY_DIR}/MLTConfigVersion.cmake
#    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mlt
#    COMPONENT Development)
