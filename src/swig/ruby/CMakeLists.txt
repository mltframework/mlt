set_source_files_properties(../mlt.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES ON CPLUSPLUS ON)

swig_add_library(mltruby LANGUAGE ruby SOURCES ../mlt.i)
target_compile_options(mltruby PRIVATE ${MLT_COMPILE_OPTIONS})
target_link_libraries(mltruby PRIVATE mlt++)
target_include_directories(mltruby PRIVATE ${RUBY_INCLUDE_DIRS})
set_target_properties(mltruby PROPERTIES OUTPUT_NAME "mlt")

install(TARGETS mltruby DESTINATION ${CMAKE_INSTALL_LIBDIR}/ruby/vendor_ruby)
