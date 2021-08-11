#include <iostream>
#include <string_view>

#include "shared_memory.hpp"

namespace sm {
namespace {
#ifdef _WIN32
constexpr std::wstring_view semaphoreName{L"Global\\MY_SHARED_MEMORY_SEMAPHORE"};

[[nodiscard]] HANDLE create(LPCWSTR name, DWORD size)
{
  const HANDLE handle{CreateFileMappingW(
    /* hFile */ INVALID_HANDLE_VALUE,
    /* lpFileMappingAttributes */ nullptr,
    /* flProtect */ PAGE_READWRITE,
    /* dwMaximumSizeHigh */ 0,
    /* dwMaximumSizeLow */ size,
    /* lpName */ name)};

  if (handle == nullptr) {
    throw std::runtime_error{"Server: CreateFileMappingW failed!"};
  }

  return handle;
}

[[nodiscard]] HANDLE attach(LPCWSTR name)
{
  const HANDLE handle{OpenFileMappingW(
    /* dwDesiredAccess */ FILE_MAP_ALL_ACCESS,
    /* bInheritHandle */ FALSE,
    /* lpName */ name)};

  if (handle == nullptr) {
    throw std::runtime_error{
      "Client: Could not attach to shared memory segment."};
  }

  return handle;
}

[[nodiscard]] HANDLE createSemaphore(LPCWSTR name)
{
  const HANDLE hSemaphore{
    CreateSemaphoreW(
      /* lpSemaphoreAttributes */ nullptr,
      /* lInitialCount */ 0,
      /* lMaximumCount */ 1,
      /* lpName */ name
    )
  };

  return hSemaphore;
}
#endif
} // anonymous namespace

SharedMemory::SharedMemory(
  Mode                   mode,
  SharedMemoryIdentifier identifier,
  std::size_t            byteCount)
  : m_mode{mode}, m_memory{nullptr}, m_byteCount
{
  byteCount
}
#ifdef _WIN32
, m_hMapFile{INVALID_HANDLE_VALUE}, m_hSemaphore
{
  INVALID_HANDLE_VALUE
}
#endif
{
#ifdef _WIN32
  if (m_mode == Mode::Create) {
    m_hMapFile
      = create(identifier.name().c_str(), static_cast<DWORD>(m_byteCount));
  }
  else if (m_mode == Mode::Attach) {
    m_hMapFile = attach(identifier.name().c_str());
  }

  m_memory = MapViewOfFile(
    /* hFileMappingObject */ m_hMapFile,
    /* dwDesiredAccess */ FILE_MAP_ALL_ACCESS,
    /* dwFileOffsetHigh */ 0,
    /* dwFileOffsetLow */ 0,
    /* dwNumberOfBytesToMap */ m_byteCount);

  if (m_memory == nullptr) {
    CloseHandle(/* hObject */ m_hMapFile);
    throw std::runtime_error{"Server: MapViewOfFile failed!"};
  }

  m_hSemaphore = createSemaphore(semaphoreName.data());

  if (m_hSemaphore == nullptr) {
    UnmapViewOfFile(/* lpBaseAddress */ m_memory);
    CloseHandle(/* hObject */ m_hMapFile);
    throw std::runtime_error{"Server: Could not create mutex!"};
  }
#endif
}

SharedMemory::~SharedMemory()
{
#ifdef _WIN32
  if (!CloseHandle(/* hObject */ m_hSemaphore)) {
    std::wcerr << L"~SharedMemory(): CloseHandle failed for m_hSemaphore.\n";
  }

  if (!UnmapViewOfFile(/* lpBaseAddress */ m_memory)) {
    std::wcerr << L"~SharedMemory(): UnmapViewOfFile failed!\n";
  }

  if (!CloseHandle(/* hObject */ m_hMapFile)) {
    std::wcerr << L"~SharedMemory(): CloseHandle failed!\n";
  }
#endif
}

std::size_t SharedMemory::size() const noexcept
{
  return m_byteCount;
}

bool SharedMemory::write(
  std::ptrdiff_t offset,
  const void*    data,
  std::size_t    byteCount)
{
#ifdef _WIN32
  if ((offset + byteCount) > m_byteCount) {
    return false;
  }

  if (!ReleaseSemaphore(
          /* hSemaphore */ m_hSemaphore, 
          /* lReleaseCount */ 1, 
          /* lpPreviousCount */ nullptr)) {
    return false;
  }

    std::byte* const baseAddress{static_cast<std::byte*>(m_memory)};

    std::byte* const startAddress{baseAddress + offset};

    CopyMemory(
      /* Destination */ startAddress,
      /* Source */ data,
      /* Length */ byteCount);

  return true;
#endif
}

bool SharedMemory::read(
  std::ptrdiff_t offset,
  void*          buffer,
  std::size_t    bytesToRead) const
{
#ifdef _WIN32
  if ((offset + bytesToRead) > m_byteCount) {
    return false;
  }

  const DWORD waitResult{WaitForSingleObject(
    /* hHandle */ m_hSemaphore,
    /* dwMilliseconds */ INFINITE)};

  switch (waitResult) {
  case WAIT_OBJECT_0: {
    const std::byte* const baseAddress{static_cast<std::byte*>(m_memory)};

    const std::byte* const startAddress{baseAddress + offset};

    CopyMemory(
      /* Destination */ buffer,
      /* Source */ startAddress,
      /* Length */ bytesToRead);
      
    break;
  }
  case WAIT_ABANDONED:
    [[fallthrough]];
  case WAIT_TIMEOUT:
    [[fallhtrough]];
  case WAIT_FAILED:
    return false;
  }

  return true;

#endif
}
} // namespace sm
