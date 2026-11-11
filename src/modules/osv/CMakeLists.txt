add_library(mltosv MODULE
    factory.c
    producer_osv.cpp
    osv_vulkan.cpp
    osv_metadata.cpp
)

include(GenerateExportHeader)
generate_export_header(mltosv)
target_compile_options(mltosv PRIVATE ${MLT_COMPILE_OPTIONS})

target_include_directories(mltosv SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_include_directories(mltosv PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mltosv PRIVATE
    mlt
    Threads::Threads
    Vulkan::Vulkan
    ${FFMPEG_LIBRARIES}
)

if(NOT MSVC)
    target_link_libraries(mltosv PRIVATE m)
endif()

set_target_properties(mltosv PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltosv LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})
