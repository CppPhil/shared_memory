#include <iostream>
#include <string_view>

#include "format_windows_error.hpp"
#include "shared_memory.hpp"

namespace sm {
namespace {
#ifdef _WIN32
constexpr std::wstring_view semaphoreName{
  L"Global\\MY_SHARED_MEMORY_SEMAPHORE"};

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
      L"Server: CreateFileMappingW failed, error message: "
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
      L"Client: OpenFileMappinW failed, error message: "
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
}
#else
// TODO: HERE

const char* mapMode(SharedMemory::Mode mode)
{
  if (mode == SharedMemory::Mode::Create) {
    return "server";
  }
  else if (mode == SharedMemory::Mode::Attach) {
    return "client";
  }
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

    throw WindowsException{
      mapMode(m_mode) + std::wstring{L": MapViewOfFile failed, error message: "}
      + formatWindowsError(GetLastError())};
  }

  m_hSemaphore = createSemaphore(semaphoreName.data());

  if (m_hSemaphore == nullptr) {
    UnmapViewOfFile(/* lpBaseAddress */ m_memory);
    CloseHandle(/* hObject */ m_hMapFile);

    throw WindowsException{
      mapMode(m_mode)
      + std::wstring{L": CreateSemaphoreW failed, error message: "}
      + formatWindowsError(GetLastError())};
  }
#endif
}

SharedMemory::~SharedMemory()
{
#ifdef _WIN32
  if (!CloseHandle(/* hObject */ m_hSemaphore)) {
    std::wcerr << mapMode(m_mode) << L": "
               << L"~SharedMemory(): CloseHandle failed for m_hSemaphore, "
                  L"error message: "
               << formatWindowsError(GetLastError()) << L'\n';
  }

  if (!UnmapViewOfFile(/* lpBaseAddress */ m_memory)) {
    std::wcerr << mapMode(m_mode) << L": "
               << L"~SharedMemory(): UnmapViewOfFile failed, error message: "
               << formatWindowsError(GetLastError()) << L'\n';
  }

  if (!CloseHandle(/* hObject */ m_hMapFile)) {
    std::wcerr << mapMode(m_mode) << L": "
               << L"~SharedMemory(): CloseHandle failed, error message: "
               << formatWindowsError(GetLastError()) << L'\n';
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
    std::wcerr << mapMode(m_mode) << L": ReleaseSemaphore failed in write: "
               << formatWindowsError(GetLastError()) << L'\n';
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
    std::wcerr << mapMode(m_mode)
               << L": SharedMemory::read: WaitForSingleObjcet resulted in "
                  L"WAIT_ABANDONED!\n";
    return false;
  case WAIT_TIMEOUT:
    std::wcerr << mapMode(m_mode)
               << L": SharedMemory::read: WaitForSingleObjcet resulted in "
                  L"WAIT_TIMEOUT!\n";
    return false;
  case WAIT_FAILED:
    std::wcerr << mapMode(m_mode)
               << L": SharedMemory::read: WaitForSingleObjcet resulted in "
                  L"WAIT_TIMEOUT, error message: "
               << formatWindowsError(GetLastError()) << L'\n';
    return false;
  }

  return true;

#endif
}
} // namespace sm
