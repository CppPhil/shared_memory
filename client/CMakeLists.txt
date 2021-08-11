add_executable(
  client
  src/main.cpp)

target_link_libraries(
  client
  PRIVATE
  shared_memory_lib)

if(UNIX)
  set_target_properties(
    client
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/apps)
endif()

