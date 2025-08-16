# For FFTW3 pkg-config is the most reliable option on most plattforms except MSVC
if(NOT MSVC)
    find_package(PkgConfig)

    if(PkgConfig_FOUND)
        pkg_check_modules(FFTW3 IMPORTED_TARGET QUIET fftw3)
    endif()

    if(FFTW3_FOUND AND NOT TARGET FFTW3::fftw3)
        add_library(FFTW3::fftw3 INTERFACE IMPORTED)

        if(FFTW3_INCLUDE_DIRS)
            set_property(TARGET FFTW3::fftw3 PROPERTY
                    INTERFACE_INCLUDE_DIRECTORIES "${FFTW3_INCLUDE_DIRS}")
        endif()
        if(FFTW3_LINK_LIBRARIES)
            set_property(TARGET FFTW3::fftw3 PROPERTY
                    INTERFACE_LINK_LIBRARIES "${FFTW3_LINK_LIBRARIES}")
        endif()
        if(FFTW3_LDFLAGS_OTHER)
            set_property(TARGET FFTW3::fftw3 PROPERTY
                    INTERFACE_LINK_OPTIONS "${FFTW3_LDFLAGS_OTHER}")
        endif()
        if(FFTW3_CFLAGS_OTHER)
            set_property(TARGET FFTW3::fftw3 PROPERTY
                    INTERFACE_COMPILE_OPTIONS "${FFTW3_CFLAGS_OTHER}")
        endif()
    endif()
endif()

# If we didn't find FFTW3 yet, try find_package
# Note: FFTW3 supports different build systems and the necessary bits for
# find_package to work are only installed if CMake was used to build FFTW3
if(NOT TARGET FFTW3::fftw3)
    find_package(FFTW3 CONFIG)
endif()
