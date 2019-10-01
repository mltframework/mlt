if(GPL)
    find_package(Qt5 COMPONENTS Core Gui Xml Svg Widgets)
    if(Qt5_FOUND)
        set(mltqt_src
            factory.c producer_qimage.c producer_kdenlivetitle.c
            common.cpp graph.cpp
            qimage_wrapper.cpp kdenlivetitle_wrapper.cpp
            filter_audiowaveform.cpp filter_qtext.cpp filter_qtblend.cpp
            producer_qtext.cpp transition_qtblend.cpp
            consumer_qglsl.cpp)
        set(mltqt_lib mlt++ mlt m Threads::Threads
            Qt5::Core Qt5::Gui Qt5::Xml Qt5::Svg Qt5::Widgets)
        set(mltqt_def USE_QT_OPENGL)
        if(GPL3)
            list(APPEND mltqt_src transition_vqm.cpp)
            list(APPEND mltqt_def GPL3)
        endif()
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
            list(APPEND mltqt_src
                filter_lightshow.cpp filter_audiospectrum.cpp)
            list(APPEND mltqt_lib fftw)
            list(APPEND mltqt_def USE_FFTW)
        endif()
        pkg_check_modules(libexif IMPORTED_TARGET libexif)
        if(TARGET PkgConfig::libexif)
            list(APPEND mltqt_lib PkgConfig::libexif)
            list(APPEND mltqt_def USE_EXIF)
        endif()
        add_library(mltqt MODULE ${mltqt_src})
        target_link_libraries(mltqt ${mltqt_lib})
        target_compile_definitions(mltqt PRIVATE ${mltqt_def})
        set_target_properties(mltqt PROPERTIES CXX_STANDARD 11)
        install(TARGETS mltqt LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
        file(GLOB yml *.yml)
        install(FILES ${yml}
                DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/qt)
    endif()
endif()
