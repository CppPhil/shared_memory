add_executable(
  server
  src/main.cpp)

target_link_libraries(
  server
  PRIVATE
  shared_memory_lib)

if(UNIX)
  set_target_properties(
    server
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/apps)
endif()

