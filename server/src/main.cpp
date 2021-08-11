#include <cstdlib>

#include <algorithm>
#include <array>
#include <iostream>

#include "shared_memory.hpp"
#include "windows_exception.hpp"

int main()
{
  std::cout << "Shared memory server started\n";

  try {
#ifdef _WIN32
    const sm::SharedMemoryIdentifier identifier{
      std::wstring{sm::sharedMemoryName}};
#else
    const sm::SharedMemoryIdentifier identifier{
    	std::string{sm::ftokFilePath}, sm::projectId
    };
#endif

    sm::SharedMemory sharedMemory{
      sm::SharedMemory::Mode::Create, identifier, sm::defaultSharedMemorySize};

    std::array<char, sm::defaultSharedMemorySize> buffer{};

    if (!sharedMemory.read(0, buffer.data(), buffer.size())) {
      std::cerr << "Server: could not read\n!";
      return EXIT_FAILURE;
    }

    for (std::size_t i{0}; i < buffer.size(); ++i) {
      if (i % 2 == 0) {
        buffer[i] &= ~0x20; /* uppercase */
      }
    }

    std::reverse(buffer.begin(), buffer.end());

    std::cout << "Server got result: ";

    for (char c : buffer) {
      std::cout << c;
    }

    std::cout << '\n';
  }
  catch (const sm::WindowsException& ex) {
    std::wcerr << L"Server caught WindowsException: " << ex.wideWhat() << L'\n';
    return EXIT_FAILURE;
  }
  catch (const std::runtime_error& ex) {
    std::cerr << "Server caught runtime_error: " << ex.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
