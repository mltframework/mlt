set_source_files_properties(../mlt.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES ON CPLUSPLUS ON)

swig_add_library(mltlua LANGUAGE lua SOURCES ../mlt.i)
target_compile_options(mltlua PRIVATE ${MLT_COMPILE_OPTIONS})
target_link_libraries(mltlua PRIVATE mlt mlt++)
target_include_directories(mltlua PRIVATE ${LUA_INCLUDE_DIR})
set_target_properties(mltlua PROPERTIES OUTPUT_NAME "mlt")
set_target_properties(mltlua PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lua")
