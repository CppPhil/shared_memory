#include <cstdlib>

#include <iostream>
#include <algorithm>
#include <array>

#include "shared_memory.hpp"

int main()
{
  try {
#ifdef _WIN32
  const sm::SharedMemoryIdentifier identifier{
    std::wstring{sm::sharedMemoryName}
  };
#endif

  sm::SharedMemory sharedMemory{
    sm::SharedMemory::Mode::Create,
    identifier,
    sm::defaultSharedMemorySize
  };

  std::array<char, sm::defaultSharedMemorySize> buffer{};

  if (!sharedMemory.read(0, buffer.data(), buffer.size())) {
    std::cerr << "Server: could not read\n!";
    return EXIT_FAILURE;
  }

  std::reverse(buffer.begin(), buffer.end());

  std::cout << "Server got result: ";

  for (char c : buffer) {
    std::cout << c;
  }

    std::cout << '\n';
  } catch (const std::runtime_error& ex) {
    std::cerr << "Server caught runtime_error: " << ex.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
