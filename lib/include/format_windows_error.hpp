#ifndef INCG_SM_FORMAT_WINDOWS_ERROR_HPP
#define INCG_SM_FORMAT_WINDOWS_ERROR_HPP
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32

namespace sm {
#ifdef _WIN32
std::wstring formatWindowsError(DWORD errorCode);
#endif // _WIN32
} // namespace sm
#endif // INCG_SM_FORMAT_WINDOWS_ERROR_HPP
