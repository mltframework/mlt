if(NOT MOD_FEEDS)
    return()
endif()

file(GLOB PAL PAL/*.*)
install(FILES ${PAL} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/feeds/PAL)
file(GLOB NTSC NTSC/*.*)
install(FILES ${NTSC} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/feeds/NTSC)
