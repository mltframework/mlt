# FFTW3 supports different build systems and the necessary
# bits for find_package are only installed if CMake was used

# First try to find FFTW3 using find_package
find_package(FFTW3 CONFIG QUIET)

if(NOT FFTW3_FOUND)
    # Fallback to pkg-config
    find_package(PkgConfig)

    if(PkgConfig_FOUND)
        pkg_check_modules(FFTW3 IMPORTED_TARGET QUIET fftw3)
    endif()
endif()

if(NOT TARGET FFTW3::fftw3)
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
