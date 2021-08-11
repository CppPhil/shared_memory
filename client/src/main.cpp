#include <cstdlib>
#include <cstddef>

#include <stdexcept>
#include <iostream>
#include <array>
#include <string>

#include "shared_memory.hpp"

int main()
{
  try {
#ifdef _WIN32
    const sm::SharedMemoryIdentifier identifier{
      std::wstring{sm::sharedMemoryName}
    };

    sm::SharedMemory sharedMemory{
      sm::SharedMemory::Mode::Attach,
      identifier,
      sm::defaultSharedMemorySize
    };

    static const std::string alphabet{
      "abcdefghijklmnopqrstuvwxyz"
    };
    std::array<char, sm::defaultSharedMemorySize> array{};

    for (std::size_t i{0}; i < array.size(); ++i) {
      array[i] = alphabet[i % alphabet.size()];
    }

    if (!sharedMemory.write(0, array.data(), array.size())) {
      std::cerr << "Client: could not write!\n";
      return EXIT_FAILURE;
    }
#endif
  } catch (const std::runtime_error& ex) {
    std::cerr << "Client caught runtime_error: " << ex.what() << '\n';
    return EXIT_FAILURE;
  } 

  return EXIT_SUCCESS;
}
