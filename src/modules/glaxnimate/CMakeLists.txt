set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CORE_DIR glaxnimate/src/core/)
set(APP_DIR glaxnimate/external/QtAppSetup/src/)

set(GLAX_SOURCES

  ${APP_DIR}app/application.cpp
  ${APP_DIR}app/settings/settings_group.cpp
  ${APP_DIR}app/settings/settings.cpp
  ${APP_DIR}app/settings/palette_settings.cpp
  ${APP_DIR}app/settings/keyboard_shortcuts.cpp
  ${APP_DIR}app/settings/keyboard_shortcuts_model.cpp
  ${APP_DIR}app/translation_service.cpp
  ${APP_DIR}app/scripting/script_engine.cpp
  ${APP_DIR}app/widgets/settings_dialog.cpp
  ${APP_DIR}app/widgets/settings_dialog.ui
  ${APP_DIR}app/widgets/widget_palette_editor.cpp
  ${APP_DIR}app/widgets/widget_palette_editor.ui
  ${APP_DIR}app/widgets/clearable_keysequence_edit.cpp
  ${APP_DIR}app/widgets/clearable_keysequence_edit.ui
  ${APP_DIR}app/widgets/keyboard_settings_widget.cpp
  ${APP_DIR}app/widgets/keyboard_settings_widget.ui
  ${APP_DIR}app/log/logger.cpp
  ${APP_DIR}app/log/log_model.cpp
  ${APP_DIR}app/cli.cpp

  ${CORE_DIR}app_info.cpp

  ${CORE_DIR}command/structure_commands.cpp
  ${CORE_DIR}command/shape_commands.cpp
  ${CORE_DIR}command/animation_commands.cpp

  ${CORE_DIR}io/base.cpp
  ${CORE_DIR}io/binary_stream.cpp
  ${CORE_DIR}io/utils.cpp
  ${CORE_DIR}io/glaxnimate/glaxnimate_format.cpp
  ${CORE_DIR}io/glaxnimate/glaxnimate_importer.cpp
  ${CORE_DIR}io/glaxnimate/glaxnimate_mime.cpp
  ${CORE_DIR}io/lottie/cbor_write_json.cpp
  ${CORE_DIR}io/lottie/lottie_format.cpp
  ${CORE_DIR}io/lottie/lottie_html_format.cpp
  ${CORE_DIR}io/lottie/tgs_format.cpp
  ${CORE_DIR}io/lottie/validation.cpp
  ${CORE_DIR}io/mime/mime_serializer.cpp
  ${CORE_DIR}io/raster/raster_format.cpp
  ${CORE_DIR}io/raster/spritesheet_format.cpp
  ${CORE_DIR}io/rive/rive_format.cpp
  ${CORE_DIR}io/rive/rive_html_format.cpp
  ${CORE_DIR}io/rive/rive_loader.cpp
  ${CORE_DIR}io/rive/rive_serializer.cpp
  ${CORE_DIR}io/rive/type_def.cpp
  ${CORE_DIR}io/rive/type_system.cpp
  ${CORE_DIR}io/svg/detail.cpp
  ${CORE_DIR}io/svg/svg_format.cpp
  ${CORE_DIR}io/svg/svg_parser.cpp
  ${CORE_DIR}io/svg/svg_renderer.cpp
  ${CORE_DIR}io/avd/avd_parser.cpp
  ${CORE_DIR}io/avd/avd_format.cpp
  ${CORE_DIR}io/avd/avd_renderer.cpp
  ${CORE_DIR}io/aep/aep_format.cpp
  ${CORE_DIR}io/aep/aep_loader.cpp
  ${CORE_DIR}io/aep/string_decoder.cpp
  ${CORE_DIR}io/aep/gradient_xml.cpp

  ${CORE_DIR}math/geom.cpp
  ${CORE_DIR}math/polynomial.cpp
  ${CORE_DIR}math/ellipse_solver.cpp
  ${CORE_DIR}math/bezier/bezier.cpp
  ${CORE_DIR}math/bezier/point.cpp
  ${CORE_DIR}math/bezier/operations.cpp
  ${CORE_DIR}math/bezier/cubic_struts.cpp
  ${CORE_DIR}math/bezier/meta.cpp
  ${CORE_DIR}math/bezier/bezier_length.cpp

  ${CORE_DIR}model/document.cpp
  ${CORE_DIR}model/document_node.cpp
  ${CORE_DIR}model/object.cpp
  ${CORE_DIR}model/transform.cpp
  ${CORE_DIR}model/factory.cpp
  ${CORE_DIR}model/animation_container.cpp
  ${CORE_DIR}model/stretchable_time.cpp
  ${CORE_DIR}model/comp_graph.cpp
  ${CORE_DIR}model/mask_settings.cpp
  ${CORE_DIR}model/visitor.cpp
  ${CORE_DIR}model/custom_font.cpp

  ${CORE_DIR}model/animation/keyframe_transition.cpp
  ${CORE_DIR}model/animation/animatable.cpp
  ${CORE_DIR}model/animation/animatable_path.cpp
  ${CORE_DIR}model/property/property.cpp
  ${CORE_DIR}model/property/reference_property.cpp
  ${CORE_DIR}model/property/option_list_property.cpp

  ${CORE_DIR}model/assets/assets.cpp
  ${CORE_DIR}model/assets/brush_style.cpp
  ${CORE_DIR}model/assets/named_color.cpp
  ${CORE_DIR}model/assets/bitmap.cpp
  ${CORE_DIR}model/assets/gradient.cpp
  ${CORE_DIR}model/assets/asset_base.cpp
  ${CORE_DIR}model/assets/asset.cpp
  ${CORE_DIR}model/assets/composition.cpp
  ${CORE_DIR}model/assets/embedded_font.cpp
  ${CORE_DIR}model/assets/network_downloader.cpp

  ${CORE_DIR}model/shapes/shape.cpp
  ${CORE_DIR}model/shapes/fill.cpp
  ${CORE_DIR}model/shapes/rect.cpp
  ${CORE_DIR}model/shapes/group.cpp
  ${CORE_DIR}model/shapes/ellipse.cpp
  ${CORE_DIR}model/shapes/path.cpp
  ${CORE_DIR}model/shapes/stroke.cpp
  ${CORE_DIR}model/shapes/polystar.cpp
  ${CORE_DIR}model/shapes/styler.cpp
  ${CORE_DIR}model/shapes/layer.cpp
  ${CORE_DIR}model/shapes/image.cpp
  ${CORE_DIR}model/shapes/precomp_layer.cpp
  ${CORE_DIR}model/shapes/text.cpp
  ${CORE_DIR}model/shapes/repeater.cpp
  ${CORE_DIR}model/shapes/trim.cpp
  ${CORE_DIR}model/shapes/inflate_deflate.cpp
  ${CORE_DIR}model/shapes/path_modifier.cpp
  ${CORE_DIR}model/shapes/round_corners.cpp
  ${CORE_DIR}model/shapes/offset_path.cpp
  ${CORE_DIR}model/shapes/zig_zag.cpp

  ${CORE_DIR}plugin/plugin.cpp
  ${CORE_DIR}plugin/action.cpp
  ${CORE_DIR}plugin/io.cpp

  ${CORE_DIR}utils/gzip.cpp
)

file(GLOB YML "*.yml")
add_custom_target(Other_glaxnimate_Files SOURCES
  ${YML}
)

function(mlt_add_glaxnimate_module ARG_TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "QT_VERSION;DATADIR" "")


    if ("${ARG_TARGET}" STREQUAL "")
        message(FATAL_ERROR "mlt_add_glaxnimate_module called without a valid target name.")
    endif()

    if (NOT (("${ARG_QT_VERSION}" STREQUAL "5") OR ("${ARG_QT_VERSION}" STREQUAL "6")))
        message(FATAL_ERROR "mlt_add_glaxnimate_module called without a valid Qt Version (allowed are 5 or 6).")
    endif()

    if ("${ARG_DATADIR}" STREQUAL "")
        message(FATAL_ERROR "mlt_add_glaxnimate_module called without a valid data dir name.")
    endif()

    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/${CORE_DIR}application_info_generated.in.hpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/${CORE_DIR}application_info_generated.hpp"
    )

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${APP_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${CORE_DIR})
    include_directories(${LibArchive_INCLUDE_DIRS})

    add_library(${ARG_TARGET} MODULE
      producer_glaxnimate.cpp
      ${GLAX_SOURCES}
    )

    target_compile_options(${ARG_TARGET} PRIVATE ${MLT_COMPILE_OPTIONS})
    add_definitions(-DWITHOUT_POTRACE -DWITHOUT_QT_COLOR_WIDGETS)

    find_package(LibArchive REQUIRED)
    find_package(ZLIB REQUIRED)

    set_property(TARGET ${ARG_TARGET} APPEND PROPERTY AUTOMOC_MACRO_NAMES "GLAXNIMATE_OBJECT")

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
      ${LibArchive_LIBRARIES}
      ZLIB::ZLIB
    )

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

if(MOD_GLAXNIMATE)
    mlt_add_glaxnimate_module(mltglaxnimate QT_VERSION 5 DATADIR glaxnimate)
endif()

if(MOD_GLAXNIMATE_QT6)
    mlt_add_glaxnimate_module(mltglaxnimate-qt6 QT_VERSION 6 DATADIR glaxnimate-qt6)
endif()
