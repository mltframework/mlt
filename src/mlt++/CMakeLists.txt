set(MLTPP_PUPLIC_HEADERS
  Mlt.h
  MltAnimation.h
  MltAudio.h
  MltChain.h
  MltConfig.h
  MltConsumer.h
  MltDeque.h
  MltEvent.h
  MltFactory.h
  MltField.h
  MltFilter.h
  MltFilteredConsumer.h
  MltFilteredProducer.h
  MltFrame.h
  MltImage.h
  MltLink.h
  MltMultitrack.h
  MltParser.h
  MltPlaylist.h
  MltProducer.h
  MltProfile.h
  MltProperties.h
  MltPushConsumer.h
  MltRepository.h
  MltService.h
  MltTokeniser.h
  MltTractor.h
  MltTransition.h
)

add_library(mlt++ SHARED
  MltAnimation.cpp
  MltAudio.cpp
  MltChain.cpp
  MltConsumer.cpp
  MltDeque.cpp
  MltEvent.cpp
  MltFactory.cpp
  MltField.cpp
  MltFilter.cpp
  MltFilteredConsumer.cpp
  MltFilteredProducer.cpp
  MltFrame.cpp
  MltImage.cpp
  MltLink.cpp
  MltMultitrack.cpp
  MltParser.cpp
  MltPlaylist.cpp
  MltProducer.cpp
  MltProfile.cpp
  MltProperties.cpp
  MltPushConsumer.cpp
  MltRepository.cpp
  MltService.cpp
  MltTokeniser.cpp
  MltTractor.cpp
  MltTransition.cpp
)

add_library(Mlt${MLT_VERSION_MAJOR}::mlt++ ALIAS mlt++)

target_compile_options(mlt++ PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mlt++ PUBLIC mlt)

target_include_directories(mlt++ PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mlt-${MLT_VERSION_MAJOR}/mlt++>
)

set_target_properties(mlt++ PROPERTIES
  VERSION ${MLT_VERSION}
  SOVERSION ${MLT_VERSION_MAJOR}
  OUTPUT_NAME mlt++-${MLT_VERSION_MAJOR}
  PUBLIC_HEADER "${MLTPP_PUPLIC_HEADERS}"
)

if(WIN32)
  if(MINGW)
    install(FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libmlt++-${MLT_VERSION_MAJOR}.dll"
      DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RENAME libmlt++.dll
    )
    target_link_options(mlt++ PRIVATE -Wl,--output-def,libmlt++.def)
    install(FILES "${CMAKE_BINARY_DIR}/libmlt++.def" DESTINATION ${CMAKE_INSTALL_LIBDIR})
  endif()
  target_compile_definitions(mlt++ PRIVATE MLTPP_EXPORTS)
endif()

if(NOT (WIN32 OR APPLE))
  target_link_options(mlt++ PRIVATE -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/mlt++.vers)
  set_target_properties(mlt++ PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mlt++.vers)
endif()

install(TARGETS mlt++
  EXPORT MltTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mlt-${MLT_VERSION_MAJOR}/mlt++
)

configure_file(mlt++.pc.in mlt++-${MLT_VERSION_MAJOR}.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mlt++-${MLT_VERSION_MAJOR}.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  COMPONENT Development
)
