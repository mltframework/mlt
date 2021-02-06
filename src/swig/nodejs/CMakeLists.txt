set_source_files_properties(../mlt.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES ON CPLUSPLUS ON)

swig_add_library(mltnodejs LANGUAGE javascript SOURCES ../mlt.i)
target_link_libraries(mltnodejs mlt mlt++)
target_include_directories(mltnodejs PRIVATE ${NODEJS_INCLUDE_DIRS})

set_target_properties(mltnodejs PROPERTIES
  OUTPUT_NAME "mlt"
  SWIG_COMPILE_OPTIONS -node
)
