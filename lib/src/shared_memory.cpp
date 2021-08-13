#include <cerrno>
#include <cstddef>
#include <cstring>

#include <iostream>
#include <string_view>

#include "format_windows_error.hpp"
#include "shared_memory.hpp"

#ifndef _WIN32
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif // !_WIN32

#ifdef _MSC_VER
#define SM_UNREACHABLE() __assume(0)
#else
#define SM_UNREACHABLE() __builtin_unreachable()
#endif

namespace sm {
namespace {
#ifdef _WIN32
constexpr std::wstring_view semaphoreName{L"Local\\MY_SHARED_MEMORY_SEMAPHORE"};

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
    throw WindowsException{
      L"Server: CreateFileMappingW failed, error: message : "
      + formatWindowsError(GetLastError())};
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
    throw WindowsException{
      L"Client: OpenFileMappingW failed, error message: "
      + formatWindowsError(GetLastError())};
  }

  return handle;
}

[[nodiscard]] HANDLE createSemaphore(LPCWSTR name)
{
  const HANDLE hSemaphore{CreateSemaphoreW(
    /* lpSemaphoreAttributes */ nullptr,
    /* lInitialCount */ 0,
    /* lMaximumCount */ 1,
    /* lpName */ name)};

  return hSemaphore;
}

const wchar_t* mapMode(SharedMemory::Mode mode)
{
  if (mode == SharedMemory::Mode::Create) {
    return L"server";
  }
  else if (mode == SharedMemory::Mode::Attach) {
    return L"client";
  }

  SM_UNREACHABLE();
}
#else
key_t createIpcKey(std::string_view filePath, int projectId)
{
  key_t key{ftok(filePath.data(), projectId)};

  if (key == -1) {
    throw std::runtime_error{
      "ftok failed: " + std::string{std::strerror(errno)}};
  }

  return key;
}

int createSharedMemory(key_t key, std::size_t size)
{
  const int id{shmget(key, size, IPC_CREAT | IPC_EXCL | 0666)};

  if (id == -1) {
    throw std::runtime_error{
      "Server: shmget failed: " + std::string{std::strerror(errno)}};
  }

  return id;
}

int fetchSharedMemoryId(key_t key, std::size_t size)
{
  const int id{shmget(key, size, 0666)};

  if (id == -1) {
    throw std::runtime_error{
      "Client: shmget failed: " + std::string{std::strerror(errno)}};
  }

  return id;
}

const char* mapMode(SharedMemory::Mode mode)
{
  if (mode == SharedMemory::Mode::Create) {
    return "server";
  }
  else if (mode == SharedMemory::Mode::Attach) {
    return "client";
  }

  SM_UNREACHABLE();
}
#endif
} // anonymous namespace

SharedMemory::SharedMemory(
  Mode                   mode,
  SharedMemoryIdentifier identifier,
  std::size_t            byteCount)
  // clang-format off
  : m_mode{mode}
  , m_memory{nullptr}
  , m_byteCount{byteCount}
#ifdef _WIN32
  , m_hMapFile{INVALID_HANDLE_VALUE}
  , m_hSemaphore{INVALID_HANDLE_VALUE}
#else
  , m_actualByteCount{m_byteCount + sizeof(sem_t)}
  , m_sharedMemoryId{-1}
  , m_semaphore{nullptr}
#endif
// clang-format on
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
    /* hFileMapingObject */ m_hMapFile,
    /* dwDesiredAccess */ FILE_MAP_ALL_ACCESS,
    /* dwFileOffsetHigh */ 0,
    /* dwFileOffsetLow */ 0,
    /* dwNumberOfBytesToMap */ m_byteCount);

  if (m_memory == nullptr) {
    CloseHandle(m_hMapFile);

    throw WindowsException{
      mapMode(m_mode) + std::wstring{L": MapViewOfFile failed, error message: "}
      + formatWindowsError(GetLastError())};
  }

  m_hSemaphore = createSemaphore(semaphoreName.data());

  if (m_hSemaphore == nullptr) {
    UnmapViewOfFile(m_memory);
    CloseHandle(m_hMapFile);

    throw WindowsException{
      mapMode(m_mode)
      + std::wstring{L": CreateSemaphoreW failed, error message: "}
      + formatWindowsError(GetLastError())};
  }
#else
  const key_t key{createIpcKey(identifier.filePath(), identifier.projectId())};

  if (m_mode == Mode::Create) {
    m_sharedMemoryId = createSharedMemory(key, m_actualByteCount);
    void* memory{shmat(m_sharedMemoryId, nullptr, 0)};

    if (memory == reinterpret_cast<void*>(-1)) {
      shmctl(m_sharedMemoryId, IPC_RMID, nullptr);

      throw std::runtime_error{
        "Server: shmat failed: " + std::string{std::strerror(errno)}};
    }

    m_memory = memory;

    sem_t* semaphore{reinterpret_cast<sem_t*>(
      static_cast<std::byte*>(m_memory) + m_byteCount)};

    const int statusCode{sem_init(semaphore, 1, 0)};

    if (statusCode == -1) {
      shmdt(m_memory);
      shmctl(m_sharedMemoryId, IPC_RMID, nullptr);

      throw std::runtime_error{
        "Server: sem_init failed: " + std::string{std::strerror(errno)}};
    }

    m_semaphore = semaphore;
  }
  else if (m_mode == Mode::Attach) {
    m_sharedMemoryId = fetchSharedMemoryId(key, m_actualByteCount);
    void* memory{shmat(m_sharedMemoryId, nullptr, 0)};

    if (memory == reinterpret_cast<void*>(-1)) {
      throw std::runtime_error{
        "Client: shmat failed: " + std::string{std::strerror(errno)}};
    }

    m_memory = memory;

    m_semaphore = reinterpret_cast<sem_t*>(
      static_cast<std::byte*>(m_memory) + m_byteCount);
  }
#endif
}

SharedMemory::~SharedMemory()
{
#ifdef _WIN32
  if (!CloseHandle(m_hSemaphore)) {
    std::wcerr << mapMode(m_mode)
               << L": ~SharedMemory(): CloseHandle failed for m_hSemaphore, "
                  L"error message: "
               << formatWindowsError(GetLastError()) << L'\n';
  }

  if (!UnmapViewOfFile(m_memory)) {
    std::wcerr << mapMode(m_mode)
               << L": ~SharedMemory(): UnmapViewOfFile failed, error message: "
               << formatWindowsError(GetLastError()) << L'\n';
  }

  if (!CloseHandle(m_hMapFile)) {
    std::wcerr << mapMode(m_mode)
               << L": ~SharedMemory(): CloseHandle failed, error message: "
               << formatWindowsError(GetLastError()) << L'\n';
  }
#else
  if (m_mode == Mode::Create) {
    if (sem_destroy(m_semaphore) == -1) {
      std::cerr << mapMode(m_mode) << ": ~SharedMemory(): sem_destroy failed: "
                << std::strerror(errno) << '\n';
    }
  }

  if (shmdt(m_memory) == -1) {
    std::cerr << mapMode(m_mode)
              << ": ~SharedMemory(): shmdt failed: " << std::strerror(errno)
              << '\n';
  }

  if (m_mode == Mode::Create) {
    if (shmctl(m_sharedMemoryId, IPC_RMID, nullptr) == -1) {
      std::cerr << mapMode(m_mode)
                << ": ~SharedMemor(): shmctl failed: " << std::strerror(errno)
                << '\n';
    }
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
  if ((offset + byteCount) > m_byteCount) {
    return false;
  }

#ifdef _WIN32
  std::byte* const baseAddress{static_cast<std::byte*>(m_memory)};

  std::byte* const startAddress{baseAddress + offset};

  CopyMemory(
    /* Destination */ startAddress,
    /* Source */ data,
    /* Length */ byteCount);

  if (!ReleaseSemaphore(
        /* hSemaphore */ m_hSemaphore,
        /* lReleaseCount */ 1,
        /* lpPreviousCount */ nullptr)) {
    std::wcerr << mapMode(m_mode) << L": ReleaseSemaphore failed in write: "
               << formatWindowsError(GetLastError()) << L'\n';
    return false;
  }

  return true;
#else
  std::memcpy(
    /* dest */ static_cast<std::byte*>(m_memory) + offset,
    /* src */ data,
    /* count */ byteCount);

  if (sem_post(m_semaphore) == -1) {
    std::cerr << mapMode(m_mode) << ": sem_post failed in SharedMemory::write: "
              << std::strerror(errno) << '\n';

    return false;
  }

  return true;
#endif
}

bool SharedMemory::read(
  std::ptrdiff_t offset,
  void*          buffer,
  std::size_t    bytesToRead) const
{
  if ((offset + bytesToRead) > m_byteCount) {
    return false;
  }

#ifdef _WIN32
  const DWORD waitResult{WaitForSingleObject(
    /* hHandle */ m_hSemaphore,
    /* dwMilliseconds */ INFINITE)};

  switch (waitResult) {
  case WAIT_OBJECT_0: {
    const std::byte* const baseAddress{static_cast<const std::byte*>(m_memory)};

    const std::byte* const startAddress{baseAddress + offset};

    CopyMemory(
      /* Destination */ buffer,
      /* Source */ startAddress,
      /* Length */ bytesToRead);

    break;
  }
  case WAIT_ABANDONED:
    std::wcerr << mapMode(m_mode)
               << "L: SharedMemory::read: WaitForSingleObject resulted in "
                  L"WAIT_ABANDONED!\n";
    return false;
  case WAIT_TIMEOUT:
    std::wcerr << mapMode(m_mode)
               << L": SharedMemory::read: WaitForSingleObject resulted in "
                  L"WAIT_TIMEOUT!\n";
    return false;
  case WAIT_FAILED:
    std::wcerr << mapMode(m_mode)
               << L": SharedMemory::read: WaitForSingleObject resulted in "
                  L"WAIT_TIMEOUT, error message: "
               << formatWindowsError(GetLastError()) << L'\n';
    return false;
  }

  return true;
#else
  const int statusCode{sem_wait(m_semaphore)};

  if (statusCode == -1) {
    std::cerr << mapMode(m_mode)
              << ": SharedMemory::read: could not wait on semaphore: "
              << std::strerror(errno) << '\n';
    return false;
  }

  std::memcpy(
    /* dest */ buffer,
    /* src */ static_cast<const std::byte*>(m_memory) + offset,
    /* count */ bytesToRead);

  return true;
#endif
}
} // namespace sm
