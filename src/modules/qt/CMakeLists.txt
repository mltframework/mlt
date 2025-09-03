set(CMAKE_AUTOMOC ON)
include(GenerateExportHeader)
function(mlt_add_qt_module ARG_TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "QT_VERSION;DATADIR" "")

    if ("${ARG_TARGET}" STREQUAL "")
        message(FATAL_ERROR "mlt_add_qt_module called without a valid target name.")
    endif()

    if (NOT (("${ARG_QT_VERSION}" STREQUAL "5") OR ("${ARG_QT_VERSION}" STREQUAL "6")))
        message(FATAL_ERROR "mlt_add_qt_module called without a valid Qt Version (allowed are 5 or 6).")
    endif()

    if ("${ARG_DATADIR}" STREQUAL "")
        message(FATAL_ERROR "mlt_add_qt_module called without a valid data dir name.")
    endif()

    add_library(${ARG_TARGET} MODULE
      common.cpp common.h
      consumer_qglsl.cpp
      factory.c
      filter_audiolevelgraph.cpp
      filter_audiowaveform.cpp
      filter_dropshadow.cpp
      filter_gpsgraphic.cpp filter_gpsgraphic.h
      filter_gpstext.cpp
      filter_qtext.cpp
      filter_qtblend_mode.c
      filter_qtblend.cpp
      filter_qtcrop.cpp
      filter_typewriter.cpp
      gps_drawing.cpp
      gps_parser.cpp gps_parser.h
      graph.cpp graph.h
      kdenlivetitle_wrapper.cpp
      producer_kdenlivetitle.c
      producer_qimage.c
      producer_qtext.cpp
      qimage_wrapper.cpp
      transition_qtblend.cpp
      typewriter.cpp
    )

    file(GLOB YML "*.yml")
    add_custom_target(Other_${ARG_TARGET}_Files SOURCES
      ${YML}
    )


    generate_export_header(${ARG_TARGET})
    target_compile_options(${ARG_TARGET} PRIVATE ${MLT_COMPILE_OPTIONS})

    target_link_libraries(${ARG_TARGET} PRIVATE
      mlt++
      mlt
      Threads::Threads
      Qt${ARG_QT_VERSION}::Core
      Qt${ARG_QT_VERSION}::Gui
      Qt${ARG_QT_VERSION}::Xml
    )
    target_include_directories(${ARG_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    if(MSVC)
      target_link_libraries(${ARG_TARGET} PRIVATE PThreads4W::PThreads4W)
    else()
        target_link_libraries(${ARG_TARGET} PRIVATE m)
    endif()

    if(ARG_QT_VERSION EQUAL 6)
      target_link_libraries(${ARG_TARGET} PRIVATE
        Qt6::SvgWidgets
        Qt6::Core5Compat
      )
    else()
      target_link_libraries(${ARG_TARGET} PRIVATE
        Qt${ARG_QT_VERSION}::Svg
        Qt${ARG_QT_VERSION}::Widgets
      )
    endif()

    target_compile_definitions(${ARG_TARGET} PRIVATE USE_QT_OPENGL)

    if(NOT WINDOWS_DEPLOY)
      target_compile_definitions(${ARG_TARGET} PRIVATE NODEPLOY)
    endif()

    if(GPL3)
      target_sources(${ARG_TARGET} PRIVATE transition_vqm.cpp)
      target_compile_definitions(${ARG_TARGET} PRIVATE GPL3)
      install(FILES transition_vqm.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/${ARG_DATADIR})
    endif()

    if(FFTW3_FOUND)
      target_sources(${ARG_TARGET} PRIVATE filter_audiospectrum.cpp filter_lightshow.cpp)
      target_link_libraries(${ARG_TARGET} PRIVATE FFTW3::fftw3)
      target_compile_definitions(${ARG_TARGET} PRIVATE USE_FFTW)
      install(FILES filter_audiospectrum.yml filter_lightshow.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/${ARG_DATADIR})
    endif()

    if(TARGET PkgConfig::libexif)
      target_link_libraries(${ARG_TARGET} PRIVATE PkgConfig::libexif)
      target_compile_definitions(${ARG_TARGET} PRIVATE USE_EXIF)
    endif()

    set_target_properties(${ARG_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

    install(TARGETS ${ARG_TARGET} LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

    install(FILES
      filter_audiolevelgraph.yml
      filter_audiospectrum.yml
      filter_audiowaveform.yml
      filter_dropshadow.yml
      filter_gpsgraphic.yml
      filter_gpstext.yml
      filter_qtblend_mode.yml
      filter_qtblend.yml
      filter_qtcrop.yml
      filter_qtext.yml
      filter_typewriter.yml
      producer_kdenlivetitle.yml
      producer_qimage.yml
      producer_qtext.yml
      transition_qtblend.yml
      DESTINATION ${MLT_INSTALL_DATA_DIR}/${ARG_DATADIR}
    )
endfunction()

if (MOD_QT)
    mlt_add_qt_module(mltqt QT_VERSION 5 DATADIR qt)
endif()

if (MOD_QT6)
    mlt_add_qt_module(mltqt6 QT_VERSION 6 DATADIR qt6)
endif()
