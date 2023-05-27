set(SRC_FILES
  showpdfast.cpp
)

add_library(showpdfast ${SRC_FILES})

add_definitions(-DPODOFO_STATIC)
target_link_libraries(showpdfast PRIVATE podofo_static)

target_include_directories(showpdfast PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
