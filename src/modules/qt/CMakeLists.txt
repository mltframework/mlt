set(CMAKE_AUTOMOC ON)

add_library(mltqt MODULE
  factory.c
  producer_qimage.c
  producer_kdenlivetitle.c
  common.cpp graph.cpp
  qimage_wrapper.cpp
  kdenlivetitle_wrapper.cpp
  filter_audiowaveform.cpp
  filter_qtext.cpp
  filter_qtblend.cpp
  filter_qtcrop.cpp
  producer_qtext.cpp
  transition_qtblend.cpp
  consumer_qglsl.cpp
  filter_typewriter.cpp
  typewriter.cpp
)

target_link_libraries(mltqt
  mlt++
  mlt
  m
  Threads::Threads
  Qt5::Core
  Qt5::Gui
  Qt5::Xml
  Qt5::Svg
  Qt5::Widgets
)

target_compile_definitions(mltqt PRIVATE USE_QT_OPENGL)

if(NOT WINDOWS_DEPLOY)
  target_compile_definitions(mltqt PRIVATE NODEPLOY)
endif()

if(GPL3)
  target_sources(mltqt PRIVATE transition_vqm.cpp)
  target_compile_definitions(mltqt PRIVATE GPL3)
  install(FILES transition_vqm.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/qt)
endif()

if(TARGET PkgConfig::FFTW)
  target_sources(mltqt PRIVATE filter_audiospectrum.cpp filter_lightshow.cpp)
  target_link_libraries(mltqt PkgConfig::FFTW)
  target_compile_definitions(mltqt PRIVATE USE_FFTW)
  install(FILES filter_audiospectrum.yml filter_lightshow.yml DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/qt)
endif()

if(TARGET PkgConfig::libexif)
  target_link_libraries(mltqt PkgConfig::libexif)
  target_compile_definitions(mltqt PRIVATE USE_EXIF)
endif()

set_target_properties(mltqt PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltqt LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES
  filter_audiowaveform.yml
  filter_qtblend.yml
  filter_qtcrop.yml
  filter_qtext.yml
  filter_typewriter.yml
  producer_kdenlivetitle.yml
  producer_qimage.yml
  producer_qtext.yml
  transition_qtblend.yml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/qt
)
