file(GLOB mlt_src *.c)
if(WIN32)
    list(APPEND mlt_src ../win32/win32.c ../win32/strptime.c)
endif()
add_library(mlt SHARED ${mlt_src})
set_target_properties (mlt PROPERTIES SOVERSION ${MLT_VERSION} VERSION 6)
if(WIN32)
    find_package(dlfcn-win32 REQUIRED)
    find_package(iconv REQUIRED)
    target_link_libraries(mlt dlfcn-win32::dl Iconv::Iconv)
    if(NODEPLOY)
        target_compile_definitions(mlt PRIVATE NODEPLOY)
    endif()
else()
    target_link_libraries(mlt dl)
endif()
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
    # dll.a should be placed in lib directory for proper linking
    if (MSYS)
        install(TARGETS mlt ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif()
else()
    install(TARGETS mlt DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

file(GLOB mlt_head *.h)
install(FILES ${mlt_head} DESTINATION include/mlt/framework)

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
