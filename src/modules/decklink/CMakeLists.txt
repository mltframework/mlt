add_library(mltdecklink MODULE
  common.cpp
  consumer_decklink.cpp
  producer_decklink.cpp
)

target_link_libraries(mltdecklink mlt Threads::Threads)

if(WIN32)
  target_sources(mltdecklink PRIVATE win/DeckLinkAPI_i.cpp)
  target_include_directories(mltdecklink PRIVATE win)
elseif(APPLE)
  target_sources(mltdecklink PRIVATE darwin/DeckLinkAPIDispatch.cpp)
  target_include_directories(mltdecklink PRIVATE darwin)
else()
  target_sources(mltdecklink PRIVATE linux/DeckLinkAPIDispatch.cpp)
  target_include_directories(mltdecklink PRIVATE linux)
endif()

# Create module in parent directory, for the benefit of "source setenv".
set_target_properties(mltdecklink PROPERTIES LIBRARY_OUTPUT_DIRECTORY ..)

install(TARGETS mltdecklink LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mlt)

install(FILES
  consumer_decklink.yml
  producer_decklink.yml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/decklink
)
