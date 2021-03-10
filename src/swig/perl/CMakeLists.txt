set_source_files_properties(../mlt.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES ON CPLUSPLUS ON)

swig_add_library(mltperl LANGUAGE perl SOURCES ../mlt.i)
target_link_libraries(mltperl PRIVATE mlt mlt++)
target_include_directories(mltperl PRIVATE ${PERL_INCLUDE_PATH})
set_target_properties(mltperl PROPERTIES OUTPUT_NAME "mlt")

string(REGEX MATCH "lib.*" PERL_MODULE_INSTALL_DIR ${PERL_VENDORARCH})

install(TARGETS mltperl DESTINATION ${PERL_MODULE_INSTALL_DIR}/auto/mlt)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mlt.pm DESTINATION ${PERL_MODULE_INSTALL_DIR})
