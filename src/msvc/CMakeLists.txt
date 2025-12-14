add_library(msvccompat STATIC libgen.c gettimeofday.c)

target_include_directories(msvccompat PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
