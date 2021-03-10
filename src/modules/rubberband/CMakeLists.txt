add_library(mltrubberband MODULE factory.c filter_rbpitch.cpp)

target_compile_options(mltrubberband PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltrubberband PRIVATE mlt PkgConfig::rubberband)

set_target_properties(mltrubberband PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltrubberband LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES filter_rbpitch.yml DESTINATION ${MLT_INSTALL_DATA_DIR}/rubberband)
