add_library(mltdecklink MODULE
  common.cpp common.h
  consumer_decklink.cpp
  producer_decklink.cpp
)

file(GLOB YML "*.yml")
add_custom_target(Other_decklink_Files SOURCES
  ${YML}
)
include(GenerateExportHeader)
generate_export_header(mltdecklink
  EXPORT_MACRO_NAME MLTDECKLINK_EXPORT
)
target_compile_options(mltdecklink PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltdecklink PRIVATE mlt Threads::Threads)

target_include_directories(mltdecklink PRIVATE  ${CMAKE_CURRENT_BINARY_DIR})
if(WIN32)
  target_sources(mltdecklink PRIVATE win/DeckLinkAPI_i.cpp)
  target_include_directories(mltdecklink PRIVATE win)
elseif(APPLE)
  target_sources(mltdecklink PRIVATE darwin/DeckLinkAPIDispatch.cpp)
  target_include_directories(mltdecklink PRIVATE darwin)
  target_link_libraries(mltdecklink PRIVATE "-framework CoreFoundation")
else()
  target_sources(mltdecklink PRIVATE linux/DeckLinkAPIDispatch.cpp)
  target_include_directories(mltdecklink PRIVATE linux)
endif()

if(CPU_SSE)
  target_compile_definitions(mltdecklink PRIVATE USE_SSE)
endif()

set_target_properties(mltdecklink PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltdecklink LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
  consumer_decklink.yml
  producer_decklink.yml
  DESTINATION ${MLT_INSTALL_DATA_DIR}/decklink
)
