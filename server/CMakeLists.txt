add_executable(
  server
  src/main.cpp)

target_link_libraries(
  server
  PRIVATE
  shared_memory_lib)

