set(MLT_PUBLIC_HEADERS
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
  mlt_image.h
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
  mlt_image.c
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
  mlt_types.c
  mlt_version.c
)

add_custom_target("Other_mlt_Files" SOURCES
  chain_normalizers.ini
  metaschema.yaml
  mlt-framework.pc.in
  mlt.vers
)

add_library(Mlt${MLT_VERSION_MAJOR}::mlt ALIAS mlt)

target_sources(mlt PRIVATE ${MLT_PUBLIC_HEADERS})

target_compile_options(mlt PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mlt PRIVATE Threads::Threads ${CMAKE_DL_LIBS})

if(MSVC)
  target_link_libraries(mlt PRIVATE msvccompat PThreads4W::PThreads4W)
  # Explicitly include vcpkg pthread headers for MSVC
  if(DEFINED VCPKG_TARGET_TRIPLET)
    target_include_directories(mlt PRIVATE "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")
  endif()
else()
  target_link_libraries(mlt PRIVATE m)
endif()

target_include_directories(mlt PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mlt-${MLT_VERSION_MAJOR}>
)

set_target_properties(mlt PROPERTIES
  VERSION ${MLT_VERSION}
  SOVERSION ${MLT_VERSION_MAJOR}
  OUTPUT_NAME mlt-${MLT_VERSION_MAJOR}
  PUBLIC_HEADER "${MLT_PUBLIC_HEADERS}"
)

if(WIN32)
  if(MINGW)
    target_link_libraries(mlt PRIVATE ${MLT_PTHREAD_LIBS})
    target_link_options(mlt PRIVATE -Wl,--output-def,${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libmlt-${MLT_VERSION_MAJOR}.def)
    install(FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libmlt-${MLT_VERSION_MAJOR}.def" DESTINATION ${CMAKE_INSTALL_LIBDIR})
  endif()
  target_sources(mlt PRIVATE ../win32/win32.c ../win32/strptime.c)
  target_link_libraries(mlt PRIVATE Iconv::Iconv)
  if(NOT WINDOWS_DEPLOY)
    target_compile_definitions(mlt PRIVATE NODEPLOY)
  endif()
endif()

if(NOT (WIN32 OR RELOCATABLE))
  if(APPLE)
    target_compile_definitions(mlt PRIVATE PREFIX_DATA="${CMAKE_INSTALL_FULL_DATADIR}/mlt" PREFIX_LIB="${CMAKE_INSTALL_FULL_LIBDIR}/mlt")
  else()
    target_compile_definitions(mlt PRIVATE PREFIX_DATA="${CMAKE_INSTALL_FULL_DATADIR}/mlt-${MLT_VERSION_MAJOR}" PREFIX_LIB="${CMAKE_INSTALL_FULL_LIBDIR}/mlt-${MLT_VERSION_MAJOR}")
    target_link_options(mlt PRIVATE -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/mlt.vers)
    set_target_properties(mlt PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mlt.vers)
  endif()
elseif(RELOCATABLE)
  target_compile_definitions(mlt PRIVATE RELOCATABLE)
endif()

install(TARGETS mlt
  EXPORT MltTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mlt-${MLT_VERSION_MAJOR}/framework
)

install(FILES metaschema.yaml chain_normalizers.ini DESTINATION ${MLT_INSTALL_DATA_DIR})

configure_file(mlt-framework.pc.in mlt-framework-${MLT_VERSION_MAJOR}.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mlt-framework-${MLT_VERSION_MAJOR}.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  COMPONENT Development
)
