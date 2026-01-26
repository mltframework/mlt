find_package(Dirent CONFIG)

if(NOT Dirent_FOUND)
    include(FindPackageHandleStandardArgs)

    find_path(Dirent_INCLUDE_DIR
        NAMES dirent.h
    )

    find_package_handle_standard_args(Dirent
        REQUIRED_VARS
            Dirent_INCLUDE_DIR
    )

    if(Dirent_FOUND AND NOT TARGET dirent)
        add_library(dirent INTERFACE)
        set_target_properties(dirent PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Dirent_INCLUDE_DIR}"
        )
    endif()
endif()
