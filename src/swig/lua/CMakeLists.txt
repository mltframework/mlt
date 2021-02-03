set_source_files_properties(../mlt.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES ON CPLUSPLUS ON)

swig_add_library(mltlua LANGUAGE lua SOURCES ../mlt.i)
target_link_libraries(mltlua mlt mlt++)
target_include_directories(mltlua PRIVATE ${LUA_INCLUDE_DIR})
set_target_properties(mltlua PROPERTIES OUTPUT_NAME "mlt")
