if(NOT MOD_KDENLIVE)
    return()
endif()

find_package(Qt5 COMPONENTS Core Xml)
if(Qt5_FOUND)
    set(qtkdenlive_src
        filter_typewriter.cpp
        typewriter.cpp)
    set(qtkdenlive_lib Qt5::Core Qt5::Xml)
endif()
file(GLOB mltkdenlive_src *.c)
add_library(mltkdenlive MODULE ${mltkdenlive_src} ${qtkdenlive_src})
target_link_libraries(mltkdenlive mlt m ${qtkdenlive_lib})
# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltkdenlive PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)
install(TARGETS mltkdenlive LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)
file(GLOB yml *.yml)
install(FILES ${yml} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/kdenlive)
