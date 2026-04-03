set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB YML "*.yml")
add_custom_target(Other_glaxnimate_Files SOURCES
  ${YML}
)
include(GenerateExportHeader)

function(mlt_add_glaxnimate_module ARG_TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "QT_VERSION;DATADIR" "")


    if ("${ARG_TARGET}" STREQUAL "")
        message(FATAL_ERROR "mlt_add_glaxnimate_module called without a valid target name.")
    endif()

    if (NOT ("${ARG_QT_VERSION}" STREQUAL "6"))
        message(FATAL_ERROR "mlt_add_glaxnimate_module called without a valid Qt Version (allowed is 6).")
    endif()

    if ("${ARG_DATADIR}" STREQUAL "")
        message(FATAL_ERROR "mlt_add_glaxnimate_module called without a valid data dir name.")
    endif()


    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_subdirectory(glaxnimate/src/core/)

    add_library(${ARG_TARGET} MODULE producer_glaxnimate.cpp)

    string(MAKE_C_IDENTIFIER ${ARG_TARGET} EXPORT_MACRO_PREFIX)
    string(TOUPPER "${EXPORT_MACRO_PREFIX}" EXPORT_MACRO_PREFIX_UPPERCASE)
    generate_export_header(${ARG_TARGET}
        EXPORT_FILE_NAME "${EXPORT_MACRO_PREFIX}_export.h"
        EXPORT_MACRO_NAME "${EXPORT_MACRO_PREFIX_UPPERCASE}_EXPORT"
    )
    target_compile_options(${ARG_TARGET} PRIVATE ${MLT_COMPILE_OPTIONS})

    target_link_libraries(${ARG_TARGET} PRIVATE
      mlt++
      mlt
      m
      Threads::Threads
      Qt${ARG_QT_VERSION}::Core
      Qt${ARG_QT_VERSION}::Gui
      Qt${ARG_QT_VERSION}::Network
      Qt${ARG_QT_VERSION}::Widgets
      Qt${ARG_QT_VERSION}::Xml
      Glaxnimate::Core
    )
    target_include_directories(${ARG_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    if(NOT WINDOWS_DEPLOY)
      target_compile_definitions(${ARG_TARGET} PRIVATE NODEPLOY)
    endif()

    set_target_properties(${ARG_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

    install(TARGETS ${ARG_TARGET} LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

    install(FILES
      producer_glaxnimate.yml
      DESTINATION ${MLT_INSTALL_DATA_DIR}/${ARG_DATADIR}
    )
endfunction()

if(MOD_GLAXNIMATE_QT6)
    mlt_add_glaxnimate_module(mltglaxnimate-qt6 QT_VERSION 6 DATADIR glaxnimate-qt6)
endif()
