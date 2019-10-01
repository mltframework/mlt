set(mltplus_src
    consumer_blipflash.c
    factory.c
    filter_affine.c
    filter_charcoal.c
    filter_dynamictext.c
    filter_dynamic_loudness.c
    filter_invert.c
    filter_lift_gamma_gain.c
    filter_loudness.c
    filter_loudness_meter.c
    filter_lumakey.c
    filter_rgblut.c
    filter_sepia.c
    filter_spot_remover.c
    filter_text.c
    filter_timer.c
    producer_blipflash.c
    producer_count.c
    transition_affine.c)
set(mltplus_lib mlt m Threads::Threads)
find_package(FFTW QUIET)
if(TARGET FFTW3::fftw3)
    add_library(fftw ALIAS FFTW3::fftw3)
else()
    pkg_check_modules(fftw3 IMPORTED_TARGET GLOBAL fftw3)
    if(TARGET PkgConfig::fftw3)
        add_library(fftw ALIAS PkgConfig::fftw3)
    endif()
endif()
if(TARGET fftw)
    list(APPEND mltplus_src filter_dance.c filter_fft.c)
    list(APPEND mltplus_lib fftw)
endif()
pkg_check_modules(libebur128 IMPORTED_TARGET libebur128)
if(TARGET PkgConfig::libebur128)
    list(APPEND mltplus_lib PkgConfig::libebur128)
    set(mltplus_inc "")
else()
    list(APPEND mltplus_src ebur128/ebur128.c)
    set(mltplus_inc
        ${CMAKE_CURRENT_SOURCE_DIR}/ebur128
        ${CMAKE_CURRENT_SOURCE_DIR}/ebur128/queue)
endif()
add_library(mltplus MODULE ${mltplus_src})
target_link_libraries(mltplus ${mltplus_lib})
target_include_directories(mltplus PRIVATE ${mltplus_inc})
install(TARGETS mltplus LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/plus)
