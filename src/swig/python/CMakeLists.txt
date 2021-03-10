set_source_files_properties(../mlt.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES ON CPLUSPLUS ON)

swig_add_library(mltpython LANGUAGE python SOURCES ../mlt.i)
target_compile_options(mltpython PRIVATE ${MLT_COMPILE_OPTIONS})
target_link_libraries(mltpython PRIVATE mlt mlt++ Python3::Module)
set_target_properties(mltpython PROPERTIES PREFIX "_" OUTPUT_NAME "mlt")

string(REGEX MATCH "[lL]ib.*" PYTHON_MODULE_INSTALL_DIR ${Python3_SITELIB})

install(TARGETS mltpython DESTINATION ${PYTHON_MODULE_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mlt.py DESTINATION ${PYTHON_MODULE_INSTALL_DIR})
