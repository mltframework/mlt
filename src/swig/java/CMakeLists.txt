set_source_files_properties(../mlt.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES ON CPLUSPLUS ON)

swig_add_library(mltjava LANGUAGE java OUTPUT_DIR src_swig SOURCES ../mlt.i)
target_compile_options(mltjava PRIVATE ${MLT_COMPILE_OPTIONS})
target_link_libraries(mltjava PRIVATE mlt mlt++)
target_include_directories(mltjava PRIVATE ${JNI_INCLUDE_DIRS})
set_target_properties(mltjava PROPERTIES OUTPUT_NAME "mlt_java")
set_target_properties(mltjava PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/java")

add_custom_command(TARGET mltjava POST_BUILD
  COMMAND javac src_swig/*.java
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
