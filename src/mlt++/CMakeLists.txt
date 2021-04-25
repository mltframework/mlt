add_library(mlt++ SHARED
  MltAnimation.cpp
  MltAudio.cpp
  MltConsumer.cpp
  MltDeque.cpp
  MltEvent.cpp
  MltFactory.cpp
  MltField.cpp
  MltFilter.cpp
  MltFilteredConsumer.cpp
  MltFilteredProducer.cpp
  MltFrame.cpp
  MltGeometry.cpp
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

target_link_libraries(mlt++ mlt)

target_include_directories(mlt++ PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mlt++>
)

set(MLTPP_SOVERSION "3")

set_target_properties(mlt++ PROPERTIES SOVERSION ${MLTPP_SOVERSION} VERSION ${MLT_VERSION})

if(WIN32)
  target_compile_definitions(mlt++ PRIVATE MLTPP_EXPORTS)
endif()

set(MLTPP_PUPLIC_HEADERS
  Mlt.h
  MltAnimation.h
  MltAudio.h
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
  MltGeometry.h
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

set_target_properties(mlt++ PROPERTIES PUBLIC_HEADER "${MLTPP_PUPLIC_HEADERS}")

install(TARGETS mlt++
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mlt++
)

configure_file(mlt++.pc.in mlt++.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mlt++.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  COMPONENT Development
)
