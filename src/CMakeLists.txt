
set(SRC_FILES
  showpdfast.cpp
)

add_library(showpdfast ${SRC_FILES})

# Add a dummy file to 'demo' project to force building
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/../demo/dummy.cpp"
  COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}../demo/dummy.cpp"
  DEPENDS showpdfast
  VERBATIM
)

add_definitions(-DPODOFO_STATIC)
target_link_libraries(showpdfast PRIVATE podofo_static)

target_include_directories(showpdfast PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
