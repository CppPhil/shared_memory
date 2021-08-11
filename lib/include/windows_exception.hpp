#ifndef INCG_SM_WINDOWS_EXCEPTION_HPP
#define INCG_SM_WINDOWS_EXCEPTION_HPP
#include <stdexcept>
#include <string>

namespace sm {
class WindowsException : public std::runtime_error {
public:
  explicit WindowsException(std::wstring errorMessage);

  const wchar_t* wideWhat() const noexcept;

  const char* what() const noexcept override;

private:
  std::wstring m_errorMessage;
};
} // namespace sm
#endif // INCG_SM_WINDOWS_EXCEPTION_HPP
