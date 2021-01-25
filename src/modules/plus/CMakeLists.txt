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
    filter_pillar_echo.c
    filter_rgblut.c
    filter_sepia.c
    filter_spot_remover.c
    filter_text.c
    filter_timer.c
    filter_strobe.c
    producer_blipflash.c
    producer_count.c
    transition_affine.c)
set(mltplus_lib mlt m Threads::Threads)
if(TARGET PkgConfig::FFTW)
    list(APPEND mltplus_src filter_dance.c filter_fft.c)
    list(APPEND mltplus_lib PkgConfig::FFTW)
    list(APPEND mltplus_def USE_FFTW)
endif()
if(TARGET PkgConfig::libebur128)
    list(APPEND mltplus_lib PkgConfig::libebur128)
    set(mltplus_inc "")
else()
    list(APPEND mltplus_src ebur128/ebur128.c)
    set(mltplus_inc
        ${CMAKE_CURRENT_SOURCE_DIR}/ebur128
        ${CMAKE_CURRENT_SOURCE_DIR}/ebur128/queue)
    list(APPEND mltplus_def USE_INTERNAL_LIBEBUR128)
endif()
add_library(mltplus MODULE ${mltplus_src})
target_link_libraries(mltplus ${mltplus_lib})
target_include_directories(mltplus PRIVATE ${mltplus_inc})
target_compile_definitions(mltplus PRIVATE ${mltplus_def})
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltplus PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltplus LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/plus)
