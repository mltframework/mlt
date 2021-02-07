add_library(mlt SHARED
  mlt_animation.c
  mlt_audio.c
  mlt_cache.c
  mlt_chain.c
  mlt_consumer.c
  mlt_deque.c
  mlt_events.c
  mlt_factory.c
  mlt_field.c
  mlt_filter.c
  mlt_frame.c
  mlt_geometry.c
  mlt_link.c
  mlt_log.c
  mlt_luma_map.c
  mlt_multitrack.c
  mlt_parser.c
  mlt_playlist.c
  mlt_pool.c
  mlt_producer.c
  mlt_profile.c
  mlt_properties.c
  mlt_property.c
  mlt_repository.c
  mlt_service.c
  mlt_slices.c
  mlt_tokeniser.c
  mlt_tractor.c
  mlt_transition.c
  mlt_version.c
)

target_link_libraries(mlt m Threads::Threads ${CMAKE_DL_LIBS})

target_include_directories(mlt PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mlt>
)

set_target_properties(mlt PROPERTIES SOVERSION ${MLT_VERSION_MAJOR} VERSION ${MLT_VERSION})

if(WIN32)
  set_target_properties(mlt PROPERTIES OUTPUT_NAME "mlt-${MLT_VERSION_MAJOR}")
  target_sources(mlt PRIVATE ../win32/win32.c ../win32/strptime.c)
  target_link_libraries(mlt Iconv::Iconv)
  if(NODEPLOY)
    target_compile_definitions(mlt PRIVATE NODEPLOY)
  endif()
endif()

if(NOT (WIN32 OR APPLE))
  target_compile_definitions(mlt PRIVATE
    PREFIX_DATA="${CMAKE_INSTALL_FULL_DATADIR}/mlt"
    PREFIX_LIB="${CMAKE_INSTALL_FULL_LIBDIR}/mlt")
endif()

set(MLT_PUPLIC_HEADERS
  mlt.h
  mlt_animation.h
  mlt_audio.h
  mlt_cache.h
  mlt_chain.h
  mlt_consumer.h
  mlt_deque.h
  mlt_events.h
  mlt_factory.h
  mlt_field.h
  mlt_filter.h
  mlt_frame.h
  mlt_geometry.h
  mlt_link.h
  mlt_log.h
  mlt_luma_map.h
  mlt_multitrack.h
  mlt_parser.h
  mlt_playlist.h
  mlt_pool.h
  mlt_producer.h
  mlt_profile.h
  mlt_properties.h
  mlt_property.h
  mlt_repository.h
  mlt_service.h
  mlt_slices.h
  mlt_tokeniser.h
  mlt_tractor.h
  mlt_transition.h
  mlt_types.h
  mlt_version.h
)

set_target_properties(mlt PROPERTIES PUBLIC_HEADER "${MLT_PUPLIC_HEADERS}")

install(TARGETS mlt
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mlt/framework
)

install(FILES metaschema.yaml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt)

configure_file(mlt-framework.pc.in mlt-framework.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mlt-framework.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  COMPONENT Development
)
