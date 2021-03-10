set_source_files_properties(../mlt.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES ON CPLUSPLUS ON)

swig_add_library(mltsharp LANGUAGE csharp OUTPUT_DIR src_swig SOURCES ../mlt.i)
target_link_libraries(mltsharp PRIVATE mlt mlt++)

add_custom_command(TARGET mltsharp POST_BUILD
    COMMAND mcs -out:mlt-sharp.dll -target:library src_swig/*.cs
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
