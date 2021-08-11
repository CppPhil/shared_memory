set(LIB_HEADERS
    include/shared_memory_identifier.hpp
    include/shared_memory.hpp)

set(LIB_SOURCES
    src/shared_memory_identifier.cpp
    src/shared_memory.cpp)

set(LIB_NAME shared_memory_lib)

add_library(
  ${LIB_NAME}
  STATIC
  ${LIB_HEADERS}
  ${LIB_SOURCES})

target_include_directories(
  ${LIB_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
  target_link_libraries(
    ${LIB_NAME}
    PUBLIC
    Kernel32.lib)

  target_compile_definitions(
    ${LIB_NAME}
    PUBLIC
    NOMINMAX)
endif()

