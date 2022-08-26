set_source_files_properties(../mlt.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES ON CPLUSPLUS ON)

swig_add_library(mltsharp LANGUAGE csharp OUTPUT_DIR src_swig SOURCES ../mlt.i)
target_compile_options(mltsharp PRIVATE ${MLT_COMPILE_OPTIONS})
target_link_libraries(mltsharp PRIVATE mlt mlt++)
set_target_properties(mltsharp PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/csharp")

add_custom_command(TARGET mltsharp POST_BUILD
    COMMAND mcs -out:mlt-sharp.dll -target:library src_swig/*.cs
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
