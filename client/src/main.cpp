#include <cstddef>
#include <cstdlib>

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

#include "shared_memory.hpp"
#include "shared_memory_identifier.hpp"
#include "windows_exception.hpp"

int main()
{
  std::cout << "Shared memory client started.\n";

  try {
#ifdef _WIN32
    const sm::SharedMemoryIdentifier identifier{
      std::wstring{sm::sharedMemoryName}};
#else
    const sm::SharedMemoryIdentifier identifier{
      std::string{sm::ftokFilePath}, sm::projectId};
#endif

    sm::SharedMemory sharedMemory{
      sm::SharedMemory::Mode::Attach, identifier, sm::defaultSharedMemorySize};

    static const std::string alphabet{"abcdefghijklmnopqrstuvwxyz"};
    std::array<char, sm::defaultSharedMemorySize> array{};

    for (std::size_t i{0}; i < array.size(); ++i) {
      array[i] = alphabet[i % alphabet.size()];
    }

    std::cout << "Client about to write: ";

    for (char c : array) {
      std::cout << c;
    }

    std::cout << '\n';

    if (!sharedMemory.write(0, array.data(), array.size())) {
      std::cerr << "Client: could not write!\n";
      return EXIT_FAILURE;
    }
  }
  catch (const sm::WindowsException& ex) {
    std::wcerr << L"Client caught WindowsException: " << ex.wideWhat() << L'\n';
    return EXIT_FAILURE;
  }
  catch (const std::runtime_error& ex) {
    std::cerr << "Client caught runtime_error: " << ex.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
