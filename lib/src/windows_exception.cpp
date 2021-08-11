#include <utility>

#include "windows_exception.hpp"

namespace sm {
WindowsException::WindowsException(std::wstring errorMessage)
  : runtime_error{""}, m_errorMessage{std::move(errorMessage)}
{
}

const wchar_t* WindowsException::wideWhat() const noexcept
{
  return m_errorMessage.c_str();
}

const char* WindowsException::what() const noexcept
{
  return "what called on WindowsException, call wideWhat instead!";
}
} // namespace sm
