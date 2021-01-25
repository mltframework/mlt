set(mltqt_src
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
  typewriter.cpp)

set(mltqt_lib
  mlt++
  mlt
  m
  Threads::Threads
  Qt5::Core
  Qt5::Gui
  Qt5::Xml
  Qt5::Svg
  Qt5::Widgets)

set(mltqt_def USE_QT_OPENGL)

if(GPL3)
  list(APPEND mltqt_src transition_vqm.cpp)
  list(APPEND mltqt_def GPL3)
endif()

if(TARGET PkgConfig::FFTW)
  list(APPEND mltqt_src filter_lightshow.cpp filter_audiospectrum.cpp)
  list(APPEND mltqt_lib PkgConfig::FFTW)
  list(APPEND mltqt_def USE_FFTW)
endif()

if(TARGET PkgConfig::libexif)
  list(APPEND mltqt_lib PkgConfig::libexif)
  list(APPEND mltqt_def USE_EXIF)
endif()

add_library(mltqt MODULE ${mltqt_src})
target_link_libraries(mltqt ${mltqt_lib})
target_compile_definitions(mltqt PRIVATE ${mltqt_def})
set_target_properties(mltqt PROPERTIES CXX_STANDARD 11)
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltqt PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltqt LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/qt)
