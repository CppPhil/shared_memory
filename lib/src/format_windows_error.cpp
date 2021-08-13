#include <iomanip>
#include <ios>
#include <locale>
#include <sstream>

#include "format_windows_error.hpp"

namespace sm {
#ifdef _WIN32
std::wstring formatWindowsError(DWORD errorCode)
{
  LPWSTR      buffer{nullptr};
  const DWORD statusCode{FormatMessageW(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER
      | FORMAT_MESSAGE_IGNORE_INSERTS,
    nullptr,
    errorCode,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    reinterpret_cast<LPWSTR>(&buffer),
    0,
    nullptr)};

  if (statusCode == 0) {
    throw std::runtime_error{"FormatMessageW failed!"};
  }

  std::wstring result{};

  if (buffer != nullptr) {
    result = buffer;
    const HLOCAL hLocal{LocalFree(buffer)};

    if (hLocal != nullptr) {
      throw std::runtime_error{"LocalFree failed!"};
    }

    std::wostringstream woss{};
    woss.imbue(std::locale::classic());

    woss << L"error code: 0x" << std::uppercase << std::hex << std::setw(2)
         << std::setfill(L'0') << errorCode << L" message: " << result;

    return woss.str();
  }

  throw std::runtime_error{"Couldn't allocate memory for FormatMessageW!"};
}
#endif // _WIN32
} // namespace sm
