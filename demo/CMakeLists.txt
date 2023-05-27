set(DEMO_FILES
  main.cpp
  dummy.cpp
)

add_executable(demo ${DEMO_FILES})
add_dependencies(demo showpdfast)

target_link_libraries(demo PRIVATE showpdfast)
