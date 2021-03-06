#pragma once
#include <cstddef>

#include <stdexcept>

#include "shared_memory_identifier.hpp"
#include "windows_exception.hpp"

#ifdef _WIN32
#include <Windows.h>
#else
#include <semaphore.h>
#endif

namespace sm {
inline constexpr std::size_t defaultSharedMemorySize{100};

class SharedMemory {
public:
  enum class Mode { Create, Attach };

  SharedMemory(
    Mode                   mode,
    SharedMemoryIdentifier identifier,
    std::size_t            byteCount);

  SharedMemory(const SharedMemory&) = delete;

  SharedMemory& operator=(const SharedMemory&) = delete;

  ~SharedMemory();

  std::size_t size() const noexcept;

  [[nodiscard]] bool
  write(std::ptrdiff_t offset, const void* data, std::size_t byteCount);

  [[nodiscard]] bool
  read(std::ptrdiff_t offset, void* buffer, std::size_t bytesToRead) const;

private:
  Mode        m_mode;
  void*       m_memory;
  std::size_t m_byteCount;
#ifdef _WIN32
  HANDLE m_hMapFile;
  HANDLE m_hSemaphore;
#else
  std::size_t m_actualByteCount;
  int         m_sharedMemoryId;
  sem_t*      m_semaphore;
#endif
};
} // namespace sm
